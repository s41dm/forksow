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

enum TransitionType {
	TransitionType_Normal,
	TransitionType_NoReset,
	TransitionType_ForceReset,
};

struct ItemStateTransition {
	WeaponState2 next;
	TransitionType type;

	ItemStateTransition() = default;

	ItemStateTransition( WeaponState2 state ) {
		next = state;
		type = TransitionType_Normal;
	}
};

static ItemStateTransition NoReset( WeaponState2 state ) {
	ItemStateTransition t;
	t.next = state;
	t.type = TransitionType_NoReset;
	return t;
}

static ItemStateTransition ForceReset( WeaponState2 state ) {
	ItemStateTransition t;
	t.next = state;
	t.type = TransitionType_ForceReset;
	return t;
}

using ItemStateThinkCallback = ItemStateTransition ( * )( const gs_state_t * gs, WeaponState2 state, s64 state_entry_time, s64 now, SyncPlayerState * ps, const usercmd_t * cmd );

struct ItemState {
	WeaponState2 state;
	ItemStateThinkCallback think;

	ItemState( WeaponState2 s, ItemStateThinkCallback t ) {
		state = s;
		think = t;
	}
};

static ItemStateTransition GenericDelay( WeaponState2 state, s64 state_entry_time, s64 now, s64 delay, WeaponState2 next ) {
	return now - state_entry_time >= delay ? next : state;
}

template< size_t N >
constexpr static Span< const ItemState > MakeStateMachine( const ItemState ( &states )[ N ] ) {
	return Span< const ItemState >( states, N );
}

static ItemState switching_out_state =
	ItemState( WeaponState2_SwitchingOut, []( const gs_state_t * gs, WeaponState2 state, s64 state_entry_time, s64 now, SyncPlayerState * ps, const usercmd_t * cmd ) -> ItemStateTransition {
		const WeaponDef * def = GS_GetWeaponDef( ps->weapon );
		if( now - state_entry_time < def->weapondown_time )
			return state;

		ps->weapon = Weapon_None;
		return WeaponState2_SwitchWeapon;
	} );

static ItemState generic_gun_states[] = {
	ItemState( WeaponState2_SwitchWeapon, []( const gs_state_t * gs, WeaponState2 state, s64 state_entry_time, s64 now, SyncPlayerState * ps, const usercmd_t * cmd ) -> ItemStateTransition {
		if( ps->pending_weapon == Weapon_None )
			return state;

		ps->weapon = ps->pending_weapon;
		ps->pending_weapon = Weapon_None;

		return WeaponState2_SwitchingIn;
	} ),

	ItemState( WeaponState2_SwitchingIn, []( const gs_state_t * gs, WeaponState2 state, s64 state_entry_time, s64 now, SyncPlayerState * ps, const usercmd_t * cmd ) -> ItemStateTransition {
		const WeaponDef * def = GS_GetWeaponDef( ps->weapon );
		return GenericDelay( state, state_entry_time, now, def->weaponup_time, WeaponState2_Idle );
	} ),

	ItemState( WeaponState2_Idle, []( const gs_state_t * gs, WeaponState2 state, s64 state_entry_time, s64 now, SyncPlayerState * ps, const usercmd_t * cmd ) -> ItemStateTransition {
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

					switch( def->firing_mode ) {
						case FiringMode_Auto: return WeaponState2_Firing;
						case FiringMode_SemiAuto: return WeaponState2_FiringSemiAuto;
						case FiringMode_Smooth: return WeaponState2_FiringSmooth;
						case FiringMode_Clip: return WeaponState2_FiringEntireClip;
					}
				}
			}

			bool wants_reload = ( cmd->buttons & BUTTON_RELOAD ) && def->clip_size != 0 && slot->ammo < def->clip_size;
			if( wants_reload || !HasAmmo( def, slot ) ) {
				return WeaponState2_Reloading;
			}
		}

		if( cmd->buttons & BUTTON_SPECIAL ) {
			// ...
		}

		if( ps->pending_weapon != Weapon_None ) {
			return WeaponState2_SwitchingOut;
		}

		return WeaponState2_Idle;
	} ),

	ItemState( WeaponState2_Firing, []( const gs_state_t * gs, WeaponState2 state, s64 state_entry_time, s64 now, SyncPlayerState * ps, const usercmd_t * cmd ) -> ItemStateTransition {
		const WeaponDef * def = GS_GetWeaponDef( ps->weapon );
		return GenericDelay( state, state_entry_time, now, def->refire_time, WeaponState2_Idle );
	} ),

	ItemState( WeaponState2_FiringSemiAuto, []( const gs_state_t * gs, WeaponState2 state, s64 state_entry_time, s64 now, SyncPlayerState * ps, const usercmd_t * cmd ) -> ItemStateTransition {
		if( cmd->buttons & BUTTON_ATTACK )
			return state;

		return NoReset( WeaponState2_Firing );
	} ),

	ItemState( WeaponState2_FiringSmooth, []( const gs_state_t * gs, WeaponState2 state, s64 state_entry_time, s64 now, SyncPlayerState * ps, const usercmd_t * cmd ) -> ItemStateTransition {
		const WeaponDef * def = GS_GetWeaponDef( ps->weapon );
		if( now - state_entry_time < def->refire_time )
			return state;

		if( cmd->buttons & BUTTON_ATTACK ) {
			gs->api.PredictedEvent( ps->POVnum, EV_SMOOTHREFIREWEAPON, ps->weapon );

			WeaponSlot * slot = GetSelectedWeapon( ps );
			slot->ammo--;
			if( slot->ammo == 0 ) {
				gs->api.PredictedEvent( ps->POVnum, EV_NOAMMOCLICK, 0 );
				return WeaponState2_Idle;
			}

			return ForceReset( WeaponState2_FiringSmooth );
		}

		return WeaponState2_Idle;
	} ),

	// ItemState( WeaponState2_FiringEntireClip, []( const gs_state_t * gs, WeaponState2 state, s64 state_entry_time, s64 now, SyncPlayerState * ps, const usercmd_t * cmd ) -> ItemStateTransition {
	// } ),

	ItemState( WeaponState2_Reloading, []( const gs_state_t * gs, WeaponState2 state, s64 state_entry_time, s64 now, SyncPlayerState * ps, const usercmd_t * cmd ) -> ItemStateTransition {
		const WeaponDef * def = GS_GetWeaponDef( ps->weapon );
		WeaponSlot * slot = GetSelectedWeapon( ps );

		if( ( cmd->buttons & BUTTON_ATTACK ) != 0 && HasAmmo( def, slot ) ) {
			return WeaponState2_Idle;
		}

		if( ps->pending_weapon != Weapon_None ) {
			return WeaponState2_SwitchingOut;
		}

		if( now - state_entry_time < def->reload_time ) {
			return state;
		}

		if( def->staged_reloading ) {
			slot->ammo++;
			gs->api.PredictedEvent( ps->POVnum, EV_WEAPONACTIVATE, ps->weapon << 1 );
			return slot->ammo == def->clip_size ? WeaponState2_Idle : ForceReset( WeaponState2_Reloading );
		}

		slot->ammo = def->clip_size;
		gs->api.PredictedEvent( ps->POVnum, EV_WEAPONACTIVATE, ps->weapon << 1 );

		return WeaponState2_Idle;
	} ),

	switching_out_state,
};

static ItemState grenade_states[] = {
	ItemState( WeaponState2_SwitchingIn, []( const gs_state_t * gs, WeaponState2 state, s64 state_entry_time, s64 now, SyncPlayerState * ps, const usercmd_t * cmd ) -> ItemStateTransition {
		const WeaponDef * def = GS_GetWeaponDef( ps->weapon );
		return GenericDelay( state, state_entry_time, now, def->weaponup_time, WeaponState2_Cooking );
	} ),

	ItemState( WeaponState2_Cooking, []( const gs_state_t * gs, WeaponState2 state, s64 state_entry_time, s64 now, SyncPlayerState * ps, const usercmd_t * cmd ) -> ItemStateTransition {
		if( ( cmd->buttons & BUTTON_ATTACK ) == 0 ) {
			return WeaponState2_Throwing;
		}

		if( ps->pending_weapon != Weapon_None ) {
			return WeaponState2_SwitchingOut;
		}

		return state;
	} ),

	ItemState( WeaponState2_Throwing, []( const gs_state_t * gs, WeaponState2 state, s64 state_entry_time, s64 now, SyncPlayerState * ps, const usercmd_t * cmd ) -> ItemStateTransition {
		if( now - state_entry_time > 500 ) {
			gs->api.PredictedFireWeapon( ps->POVnum, Weapon_GrenadeLauncher );
			return WeaponState2_SwitchingOut;
		}

		return state;
	} ),

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

static const ItemState * FindState( Span< const ItemState > sm, WeaponState2 state ) {
	for( const ItemState & s : sm ) {
		if( s.state == state ) {
			return &s;
		}
	}

	return NULL;
}

void UpdateWeapons( const gs_state_t * gs, s64 now, SyncPlayerState * ps, const usercmd_t * cmd, int timeDelta ) {
	if( cmd->weaponSwitch != Weapon_None && GS_CanEquip( ps, cmd->weaponSwitch ) ) {
		ps->pending_weapon = cmd->weaponSwitch;
	}

	while( true ) {
		Span< const ItemState > sm = FindItemStateMachine( ps );

		WeaponState2 old_state = ps->new_weapon_state;
		const ItemState * s = FindState( sm, old_state );

		ItemStateTransition transition = s == NULL ? ItemStateTransition( sm[ 0 ].state ) : s->think( gs, old_state, ps->weapon_state_time, now, ps, cmd );

		Com_GGPrint( "{} @ {} -> {}", old_state, ps->weapon_state_time, transition.next );

		if( transition.type == TransitionType_Normal ) {
			if( old_state == transition.next )
				break;
			ps->new_weapon_state = transition.next;
			ps->weapon_state_time = now;
		}
		else if( transition.type == TransitionType_NoReset ) {
			if( old_state == transition.next )
				break;
			ps->new_weapon_state = transition.next;
		}
		else {
			ps->new_weapon_state = transition.next;
			ps->weapon_state_time = now;
		}
	}
}

bool GS_CanEquip( const SyncPlayerState * ps, WeaponType weapon ) {
	return ( ps->pmove.features & PMFEAT_WEAPONSWITCH ) != 0 && FindWeapon( ps, weapon ) != NULL;
}
