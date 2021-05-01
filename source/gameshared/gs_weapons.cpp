/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "qcommon/base.h"
#include "qcommon/qcommon.h"
#include "qcommon/rng.h"
#include "gameshared/gs_public.h"
#include "gameshared/gs_weapons.h"

void GS_TraceBullet( const gs_state_t * gs, trace_t * trace, trace_t * wallbang_trace, Vec3 start, Vec3 dir, Vec3 right, Vec3 up, Vec2 spread, int range, int ignore, int timeDelta ) {
	Vec3 end = start + dir * range + right * spread.x + up * spread.y;

	gs->api.Trace( trace, start, Vec3( 0.0f ), Vec3( 0.0f ), end, ignore, MASK_WALLBANG, timeDelta );

	if( wallbang_trace != NULL ) {
		gs->api.Trace( wallbang_trace, start, Vec3( 0.0f ), Vec3( 0.0f ), trace->endpos, ignore, MASK_SHOT, timeDelta );
	}
}

Vec2 RandomSpreadPattern( u16 entropy, float spread ) {
	RNG rng = new_rng( entropy, 0 );
	return spread * UniformSampleInsideCircle( &rng );
}

float ZoomSpreadness( s16 zoom_time, const WeaponDef * def ) {
	float frac = 1.0f - float( zoom_time ) / float( ZOOMTIME );
	return frac * def->range * atanf( Radians( def->zoom_spread ) );
}

Vec2 FixedSpreadPattern( int i, float spread ) {
	float x = i * 2.4f;
	return Vec2( cosf( x ), sinf( x ) ) * spread * sqrtf( x );
}

void GS_TraceLaserBeam( const gs_state_t * gs, trace_t * trace, Vec3 origin, Vec3 angles, float range, int ignore, int timeDelta, void ( *impact )( const trace_t * trace, Vec3 dir, void * data ), void * data ) {
	Vec3 maxs = Vec3( 0.5f, 0.5f, 0.5f );

	Vec3 dir;
	AngleVectors( angles, &dir, NULL, NULL );
	Vec3 end = origin + dir * range;

	trace->ent = 0;

	gs->api.Trace( trace, origin, -maxs, maxs, end, ignore, MASK_SHOT, timeDelta );
	if( trace->ent != -1 && impact != NULL ) {
		impact( trace, dir, data );
	}
}

WeaponSlot * FindWeapon( SyncPlayerState * player, WeaponType weapon ) {
	for( size_t i = 0; i < ARRAY_COUNT( player->weapons ); i++ ) {
		if( player->weapons[ i ].weapon == weapon ) {
			return &player->weapons[ i ];
		}
	}

	return NULL;
}

const WeaponSlot * FindWeapon( const SyncPlayerState * player, WeaponType weapon ) {
	return FindWeapon( const_cast< SyncPlayerState * >( player ), weapon );
}

WeaponSlot * GetSelectedWeapon( SyncPlayerState * player ) {
	return FindWeapon( player, player->weapon );
}

const WeaponSlot * GetSelectedWeapon( const SyncPlayerState * player ) {
	return FindWeapon( player, player->weapon );
}

static bool HasAmmo( const WeaponDef * def, const WeaponSlot * slot ) {
	return def->clip_size == 0 || slot->ammo > 0;
}

struct ItemStateTransition {
	WeaponState state;
	s64 delay;
	bool reset;

	ItemStateTransition() = default;

	ItemStateTransition( WeaponState s, s64 d = 0 ) {
		state = s;
		delay = d;
		reset = true;
	}
};

static ItemStateTransition NoReset( WeaponState state ) {
	ItemStateTransition t;
	t.state = state;
	t.reset = false;
	return t;
}

using ItemStateThinkCallback = ItemStateTransition ( * )( const gs_state_t * gs, WeaponState state, SyncPlayerState * ps, const usercmd_t * cmd );

struct ItemState {
	WeaponState state;
	ItemStateThinkCallback think;

	ItemState( WeaponState s, ItemStateThinkCallback t ) {
		state = s;
		think = t;
	}
};

static ItemStateTransition GenericDelay( WeaponState state, SyncPlayerState * ps, WeaponState next ) {
	return ps->weapon_state_time > 0 ? state : next;
}

template< size_t N >
constexpr static Span< const ItemState > MakeStateMachine( const ItemState ( &states )[ N ] ) {
	return Span< const ItemState >( states, N );
}

static void HandleZoom( const gs_state_t * gs, SyncPlayerState * ps, const usercmd_t * cmd ) {
	s16 last_zoom_time = ps->zoom_time;
	bool can_zoom = ( ps->weapon_state == WeaponState_Idle
		|| ps->weapon_state == WeaponState_Firing
		|| ps->weapon_state == WeaponState_FiringSemiAuto
		|| ps->weapon_state == WeaponState_FiringEntireClip )
		&& ( ps->pmove.features & PMFEAT_SCOPE );

	const WeaponDef * def = GS_GetWeaponDef( ps->weapon );
	if( can_zoom && def->zoom_fov != 0 && ( cmd->buttons & BUTTON_SPECIAL ) != 0 ) {
		ps->zoom_time = Min2( ps->zoom_time + cmd->msec, ZOOMTIME );
		if( last_zoom_time == 0 ) {
			gs->api.PredictedEvent( ps->POVnum, EV_ZOOM_IN, ps->weapon );
		}
	}
	else {
		ps->zoom_time = Max2( 0, ps->zoom_time - cmd->msec );
		if( ps->zoom_time == 0 && last_zoom_time != 0 ) {
			gs->api.PredictedEvent( ps->POVnum, EV_ZOOM_OUT, ps->weapon );
		}
	}
}

static ItemStateTransition AllowWeaponSwitch( const gs_state_t * gs, SyncPlayerState * ps, ItemStateTransition otherwise ) {
	if( ps->pending_weapon == Weapon_None || ps->pending_weapon == ps->weapon ) {
		ps->pending_weapon = Weapon_None;
		return otherwise;
	}

	gs->api.PredictedEvent( ps->POVnum, EV_WEAPONDROP, 0 );

	const WeaponDef * def = GS_GetWeaponDef( ps->weapon );
	return ItemStateTransition( WeaponState_SwitchingOut, def->switch_out_time );
}

static ItemState switching_out_state =
	ItemState( WeaponState_SwitchingOut, []( const gs_state_t * gs, WeaponState state, SyncPlayerState * ps, const usercmd_t * cmd ) -> ItemStateTransition {
		return GenericDelay( state, ps, WeaponState_SwitchWeapon );
	} );

static ItemState generic_gun_states[] = {
	// TODO: move this out into some dispatch state machine
	ItemState( WeaponState_SwitchWeapon, []( const gs_state_t * gs, WeaponState state, SyncPlayerState * ps, const usercmd_t * cmd ) -> ItemStateTransition {
		if( ps->pending_weapon == Weapon_None ) {
			return state;
		}

		return WeaponState_Init;
	} ),

	ItemState( WeaponState_Init, []( const gs_state_t * gs, WeaponState state, SyncPlayerState * ps, const usercmd_t * cmd ) -> ItemStateTransition {
		bool just_respawned = ps->weapon != Weapon_None;
		ps->weapon = ps->pending_weapon;
		ps->pending_weapon = Weapon_None;

		const WeaponDef * def = GS_GetWeaponDef( ps->weapon );
		if( just_respawned ) {
			gs->api.PredictedEvent( ps->POVnum, EV_WEAPONACTIVATE, ps->weapon << 1 );
			return ItemStateTransition( WeaponState_SwitchingIn, def->switch_in_time );
		}

		gs->api.PredictedEvent( ps->POVnum, EV_WEAPONACTIVATE, ( ps->weapon << 1 ) | 1 );

		// wait for the player to release mouse1 before letting them
		// shoot because we have click to respawn in warmup
		return WeaponState_FiringSemiAuto;
	} ),

	ItemState( WeaponState_SwitchingIn, []( const gs_state_t * gs, WeaponState state, SyncPlayerState * ps, const usercmd_t * cmd ) -> ItemStateTransition {
		return AllowWeaponSwitch( gs, ps, GenericDelay( state, ps, WeaponState_Idle ) );
	} ),

	ItemState( WeaponState_Idle, []( const gs_state_t * gs, WeaponState state, SyncPlayerState * ps, const usercmd_t * cmd ) -> ItemStateTransition {
		assert( ps->weapon_state_time == 0 );

		const WeaponDef * def = GS_GetWeaponDef( ps->weapon );
		WeaponSlot * slot = GetSelectedWeapon( ps );

		if( !GS_ShootingDisabled( gs ) ) {
			if( cmd->buttons & BUTTON_ATTACK ) {
				if( HasAmmo( def, slot ) ) {
					gs->api.PredictedFireWeapon( ps->POVnum, ps->weapon );

					if( def->clip_size > 0 ) {
						slot->ammo--;
						if( slot->ammo == 0 ) {
							gs->api.PredictedEvent( ps->POVnum, EV_NOAMMOCLICK, 0 );
						}
					}

					WeaponState new_state;
					switch( def->firing_mode ) {
						case FiringMode_Auto:
							new_state = WeaponState_Firing;
							break;
						case FiringMode_SemiAuto:
							new_state = WeaponState_FiringSemiAuto;
							break;
						case FiringMode_Smooth:
							new_state = WeaponState_FiringSmooth;
							break;
						case FiringMode_Clip:
							new_state = WeaponState_FiringEntireClip;
							break;
					}

					return ItemStateTransition( new_state, def->refire_time );
				}
			}

			bool wants_reload = ( cmd->buttons & BUTTON_RELOAD ) && def->clip_size != 0 && slot->ammo < def->clip_size;
			if( wants_reload || !HasAmmo( def, slot ) ) {
				return ItemStateTransition( WeaponState_Reloading, def->reload_time );
			}
		}

		return AllowWeaponSwitch( gs, ps, WeaponState_Idle );
	} ),

	ItemState( WeaponState_Firing, []( const gs_state_t * gs, WeaponState state, SyncPlayerState * ps, const usercmd_t * cmd ) -> ItemStateTransition {
		return GenericDelay( state, ps, WeaponState_Idle );
	} ),

	ItemState( WeaponState_FiringSemiAuto, []( const gs_state_t * gs, WeaponState state, SyncPlayerState * ps, const usercmd_t * cmd ) -> ItemStateTransition {
		if( cmd->buttons & BUTTON_ATTACK )
			return state;

		return NoReset( WeaponState_Firing );
	} ),

	ItemState( WeaponState_FiringSmooth, []( const gs_state_t * gs, WeaponState state, SyncPlayerState * ps, const usercmd_t * cmd ) -> ItemStateTransition {
		if( ps->weapon_state_time > 0 ) {
			return state;
		}

		WeaponSlot * slot = GetSelectedWeapon( ps );

		if( ( cmd->buttons & BUTTON_ATTACK ) == 0 || slot->ammo == 0 ) {
			return WeaponState_Idle;
		}

		gs->api.PredictedEvent( ps->POVnum, EV_SMOOTHREFIREWEAPON, ps->weapon );

		slot->ammo--;
		if( slot->ammo == 0 ) {
			gs->api.PredictedEvent( ps->POVnum, EV_NOAMMOCLICK, 0 );
		}

		const WeaponDef * def = GS_GetWeaponDef( ps->weapon );
		return ItemStateTransition( WeaponState_FiringSmooth, def->refire_time );
	} ),

	ItemState( WeaponState_FiringEntireClip, []( const gs_state_t * gs, WeaponState state, SyncPlayerState * ps, const usercmd_t * cmd ) -> ItemStateTransition {
		if( ps->weapon_state_time > 0 ) {
			return state;
		}

		WeaponSlot * slot = GetSelectedWeapon( ps );
		if( slot->ammo == 0 ) {
			return WeaponState_Idle;
		}

		gs->api.PredictedFireWeapon( ps->POVnum, ps->weapon );

		slot->ammo--;
		if( slot->ammo == 0 ) {
			gs->api.PredictedEvent( ps->POVnum, EV_NOAMMOCLICK, 0 );
		}

		const WeaponDef * def = GS_GetWeaponDef( ps->weapon );
		return ItemStateTransition( WeaponState_FiringEntireClip, def->refire_time );
	} ),

	ItemState( WeaponState_Reloading, []( const gs_state_t * gs, WeaponState state, SyncPlayerState * ps, const usercmd_t * cmd ) -> ItemStateTransition {
		const WeaponDef * def = GS_GetWeaponDef( ps->weapon );

		if( ps->pending_weapon != Weapon_None ) {
			gs->api.PredictedEvent( ps->POVnum, EV_WEAPONDROP, 0 );
			return ItemStateTransition( WeaponState_SwitchingOut, def->switch_out_time );
		}

		WeaponSlot * slot = GetSelectedWeapon( ps );

		if( ( cmd->buttons & BUTTON_ATTACK ) != 0 && HasAmmo( def, slot ) ) {
			return WeaponState_Idle;
		}

		if( ps->weapon_state_time > 0 ) {
			return state;
		}

		if( def->staged_reloading ) {
			slot->ammo++;
			gs->api.PredictedEvent( ps->POVnum, EV_WEAPONACTIVATE, ps->weapon << 1 );
			return slot->ammo == def->clip_size ? WeaponState_Idle : ItemStateTransition( WeaponState_Reloading, def->reload_time );
		}

		slot->ammo = def->clip_size;
		gs->api.PredictedEvent( ps->POVnum, EV_WEAPONACTIVATE, ps->weapon << 1 );

		return WeaponState_Idle;
	} ),

	switching_out_state,
};

static ItemState grenade_states[] = {
	ItemState( WeaponState_Init, []( const gs_state_t * gs, WeaponState state, SyncPlayerState * ps, const usercmd_t * cmd ) -> ItemStateTransition {
		return ItemStateTransition( WeaponState_SwitchingIn, 500 );
	} ),

	ItemState( WeaponState_SwitchingIn, []( const gs_state_t * gs, WeaponState state, SyncPlayerState * ps, const usercmd_t * cmd ) -> ItemStateTransition {
		return GenericDelay( state, ps, WeaponState_Cooking );
	} ),

	ItemState( WeaponState_Cooking, []( const gs_state_t * gs, WeaponState state, SyncPlayerState * ps, const usercmd_t * cmd ) -> ItemStateTransition {
		if( ( cmd->buttons & BUTTON_ATTACK ) == 0 ) {
			return ItemStateTransition( WeaponState_Throwing, 500 );
		}

		return AllowWeaponSwitch( gs, ps, state );
	} ),

	ItemState( WeaponState_Throwing, []( const gs_state_t * gs, WeaponState state, SyncPlayerState * ps, const usercmd_t * cmd ) -> ItemStateTransition {
		gs->api.PredictedFireWeapon( ps->POVnum, Weapon_GrenadeLauncher );
		return WeaponState_SwitchingOut;
	} ),

	// TODO: can't use this, need to set gagdet to none not weapon
	switching_out_state,
};

constexpr static Span< const ItemState > generic_gun_state_machine = MakeStateMachine( generic_gun_states );
constexpr static Span< const ItemState > grenade_state_machine = MakeStateMachine( grenade_states );

static Span< const ItemState > FindItemStateMachine( SyncPlayerState * ps ) {
	if( ps->weapon == Weapon_Pistol ) {
		return grenade_state_machine;
	}

	return generic_gun_state_machine;
}

static const ItemState * FindState( Span< const ItemState > sm, WeaponState state ) {
	for( const ItemState & s : sm ) {
		if( s.state == state ) {
			return &s;
		}
	}

	return NULL;
}

void UpdateWeapons( const gs_state_t * gs, SyncPlayerState * ps, const usercmd_t * cmd, int timeDelta ) {
	ps->weapon_state_time = Max2( ps->weapon_state_time - cmd->msec, 0 );

	if( cmd->weaponSwitch != Weapon_None && GS_CanEquip( ps, cmd->weaponSwitch ) ) {
		ps->pending_weapon = cmd->weaponSwitch;
	}

	HandleZoom( gs, ps, cmd );

	while( true ) {
		Span< const ItemState > sm = FindItemStateMachine( ps );

		WeaponState old_state = ps->weapon_state;
		const ItemState * s = FindState( sm, old_state );

		ItemStateTransition transition = s == NULL ? ItemStateTransition( sm[ 0 ].state ) : s->think( gs, old_state, ps, cmd );

		if( transition.reset ) {
			if( transition.state == ps->weapon_state && transition.delay == 0 )
				break;
			ps->weapon_state = transition.state;
			ps->weapon_state_time = transition.delay;
		}
		else {
			ps->weapon_state = transition.state;
		}
	}
}

bool GS_CanEquip( const SyncPlayerState * ps, WeaponType weapon ) {
	return ( ps->pmove.features & PMFEAT_WEAPONSWITCH ) != 0 && FindWeapon( ps, weapon ) != NULL;
}
