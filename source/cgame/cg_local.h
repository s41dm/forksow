/*
Copyright (C) 2002-2003 Victor Luchits

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

#include "qcommon/types.h"
#include "qcommon/qcommon.h"
#include "gameshared/gs_public.h"
#include "cgame/ref.h"

#include "client/client.h"
#include "cgame/cg_public.h"
#include "cgame/cg_syscalls.h"
#include "cgame/cg_decals.h"
#include "cgame/cg_particles.h"
#include "cgame/cg_sprays.h"

#include "client/sound.h"
#include "client/renderer/types.h"

#define VSAY_TIMEOUT 2500

constexpr float FOV = 107.9f; // chosen to upset everyone equally

constexpr RGB8 TEAM_COLORS[] = {
	RGB8( 0, 204, 255 ),
	RGB8( 255, 24, 96 ),
	RGB8( 50, 200, 90 ),
	RGB8( 210, 170, 0 ),
};

enum {
	LOCALEFFECT_EV_PLAYER_TELEPORT_IN,
	LOCALEFFECT_EV_PLAYER_TELEPORT_OUT,
	LOCALEFFECT_VSAY_TIMEOUT,
	LOCALEFFECT_ROCKETTRAIL_LAST_DROP,
	LOCALEFFECT_ROCKETFIRE_LAST_DROP,
	LOCALEFFECT_GRENADETRAIL_LAST_DROP,
	LOCALEFFECT_BLOODTRAIL_LAST_DROP,
	LOCALEFFECT_FLAGTRAIL_LAST_DROP,
	LOCALEFFECT_LASERBEAM,
	LOCALEFFECT_LASERBEAM_SMOKE_TRAIL,
	LOCALEFFECT_EV_WEAPONBEAM,
	MAX_LOCALEFFECTS = 64,
};

struct centity_t {
	SyncEntityState current;
	SyncEntityState prev;        // will always be valid, but might just be a copy of current

	int serverFrame;            // if not current, this ent isn't in the frame
	int64_t fly_stoptime;

	int64_t respawnTime;

	entity_t ent;                   // interpolated, to be added to render list
	unsigned int type;
	unsigned int effects;

	Vec3 velocity;

	bool canExtrapolate;
	bool canExtrapolatePrev;
	Vec3 prevVelocity;
	int microSmooth;
	Vec3 microSmoothOrigin;
	Vec3 microSmoothOrigin2;

	// effects
	ImmediateSoundHandle sound;
	Vec3 trailOrigin;         // for particle trails

	// local effects from events timers
	int64_t localEffects[MAX_LOCALEFFECTS];

	// attached laser beam
	Vec3 laserOrigin;
	Vec3 laserPoint;
	Vec3 laserOriginOld;
	Vec3 laserPointOld;
	ImmediateSoundHandle lg_beam_sound;

	bool linearProjectileCanDraw;
	Vec3 linearProjectileViewerSource;
	Vec3 linearProjectileViewerVelocity;

	Vec3 teleportedTo;
	Vec3 teleportedFrom;

	// used for client side animation of player models
	int lastVelocitiesFrames[4];
	Vec4 lastVelocities[4];
	bool jumpedLeft;
	Vec3 animVelocity;
	float yawVelocity;
};

#include "cgame/cg_pmodels.h"

struct cgs_media_t {
	// sounds
	const SoundEffect * sfxWeaponNoAmmo;

	const SoundEffect * sfxWeaponHit[ 4 ];
	const SoundEffect * sfxWeaponKill;
	const SoundEffect * sfxWeaponHitTeam;

	const SoundEffect * sfxItemRespawn;
	const SoundEffect * sfxTeleportIn;
	const SoundEffect * sfxTeleportOut;
	const SoundEffect * sfxShellHit;

	const SoundEffect * sfxBladeFleshHit;
	const SoundEffect * sfxBladeWallHit;

	const SoundEffect * sfxBulletImpact;
	const SoundEffect * sfxBulletWhizz;

	const SoundEffect * sfxRiotgunHit;

	const SoundEffect * sfxGrenadeBounce;
	const SoundEffect * sfxGrenadeExplosion;

	const SoundEffect * sfxRocketLauncherHit;

	const SoundEffect * sfxPlasmaHit;
	const SoundEffect * sfxBubbleHit;

	const SoundEffect * sfxLasergunHum;
	const SoundEffect * sfxLasergunBeam;
	const SoundEffect * sfxLasergunStop;
	const SoundEffect * sfxLasergunHit;

	const SoundEffect * sfxElectroboltHit;

	const SoundEffect * sfxVSaySounds[ Vsay_Total ];

	const SoundEffect * sfxSpikesArm;
	const SoundEffect * sfxSpikesDeploy;
	const SoundEffect * sfxSpikesGlint;
	const SoundEffect * sfxSpikesRetract;

	const SoundEffect * sfxFall;

	const SoundEffect * sfxTbag;
	const SoundEffect * sfxSpray;

	const SoundEffect * sfxHeadshot;

	// models
	const Model * modDash;
	const Model * modGib;

	const Model * modPlasmaExplosion;

	const Model * modBulletExplode;
	const Model * modBladeWallHit;
	const Model * modBladeWallExplo;

	const Model * modElectroBoltWallHit;

	const Model * modLasergunWallExplo;

	const Material * shaderParticle;
	const Material * shaderRocketExplosion;
	const Material * shaderRocketExplosionRing;
	const Material * shaderGrenadeExplosion;
	const Material * shaderGrenadeExplosionRing;
	const Material * shaderBulletExplosion;
	const Material * shaderWaterBubble;
	const Material * shaderSmokePuff;

	const Material * shaderSmokePuff1;
	const Material * shaderSmokePuff2;
	const Material * shaderSmokePuff3;

	const Material * shaderRocketFireTrailPuff;
	const Material * shaderGrenadeTrailSmokePuff;
	const Material * shaderRocketTrailSmokePuff;
	const Material * shaderBloodTrailPuff;
	const Material * shaderBloodTrailLiquidPuff;
	const Material * shaderBloodImpactPuff;
	const Material * shaderBombIcon;
	const Material * shaderTeleporterSmokePuff;
	const Material * shaderBladeMark;
	const Material * shaderBulletMark;
	const Material * shaderExplosionMark;
	const Material * shaderEnergyMark;
	const Material * shaderLaser;
	const Material * shaderNet;
	const Material * shaderTeleportShellGfx;

	const Material * shaderPlasmaMark;
	const Material * shaderEBBeam;
	const Material * shaderLGBeam;
	const Material * shaderTracer;
	const Material * shaderEBImpact;

	const Material * shaderPlayerShadow;

	const Material * shaderTick;

	const Material * shaderWeaponIcon[ Weapon_Count ];
	const Material * shaderKeyIcon[KEYICON_TOTAL];

	const Material * shaderAlive;
	const Material * shaderDead;
	const Material * shaderReady;
};

struct cg_clientInfo_t {
	char name[MAX_QPATH];
	int hand;
};

#define MAX_ANGLES_KICKS 3

struct cg_kickangles_t {
	int64_t timestamp;
	int64_t kicktime;
	float v_roll, v_pitch;
};

#define PREDICTED_STEP_TIME 150 // stairs smoothing time
#define MAX_AWARD_LINES 3
#define MAX_AWARD_DISPLAYTIME 5000

// view types
enum {
	VIEWDEF_DEMOCAM,
	VIEWDEF_PLAYERVIEW,
};

struct cg_viewdef_t {
	int type;
	int POVent;
	bool thirdperson;
	bool playerPrediction;
	bool drawWeapon;
	bool draw2D;
	float fov_x, fov_y;
	float fracDistFOV;
	Vec3 origin;
	Vec3 angles;
	mat3_t axis;
	Vec3 velocity;
};

#include "cgame/cg_democams.h"

// this is not exactly "static" but still...
struct cg_static_t {
	const char *serverName;
	const char *demoName;
	unsigned int playerNum;

	// fonts
	int fontSystemTinySize;
	int fontSystemSmallSize;
	int fontSystemMediumSize;
	int fontSystemBigSize;

	float textSizeTiny;
	float textSizeSmall;
	float textSizeMedium;
	float textSizeBig;

	const Font * fontMontserrat;
	const Font * fontMontserratBold;
	const Font * fontMontserratItalic;
	const Font * fontMontserratBoldItalic;

	cgs_media_t media;

	bool precacheDone;

	bool demoPlaying;
	unsigned snapFrameTime;
	unsigned extrapolationTime;

	//
	// locally derived information from server state
	//
	char configStrings[MAX_CONFIGSTRINGS][MAX_CONFIGSTRING_CHARS];
	char baseConfigStrings[MAX_CONFIGSTRINGS][MAX_CONFIGSTRING_CHARS];

	cg_clientInfo_t clientInfo[MAX_CLIENTS];

	PlayerModelMetadata *teamModelInfo[2];

	char checkname[MAX_QPATH];
};

struct cg_state_t {
	int frameCount;

	snapshot_t frame, oldFrame;
	bool fireEvents;
	bool firstFrame;

	Vec3 predictedOrigins[CMD_BACKUP];              // for debug comparing against server

	float predictedStep;                // for stair up smoothing
	int64_t predictedStepTime;

	int64_t predictingTimeStamp;
	int64_t predictedEventTimes[PREDICTABLE_EVENTS_MAX];
	Vec3 predictionError;
	SyncPlayerState predictedPlayerState;     // current in use, predicted or interpolated
	int predictedGroundEntity;

	// prediction optimization (don't run all ucmds in not needed)
	int64_t predictFrom;
	SyncEntityState predictFromEntityState;
	SyncPlayerState predictFromPlayerState;

	float lerpfrac;                     // between oldframe and frame
	float xerpTime;
	float oldXerpTime;
	float xerpSmoothFrac;

	int effects;

	bool showScoreboard;            // demos and multipov

	unsigned int multiviewPlayerNum;       // for multipov chasing, takes effect on next snap

	int pointedNum;
	int64_t pointRemoveTime;
	int pointedHealth;

	float xyspeed;

	bool recoiling;
	Vec3 recoil;
	Vec3 recoil_initial;

	float damage_effect;

	float oldBobTime;
	int bobCycle;                   // odd cycles are right foot going forward
	float bobFracSin;               // sin(bobfrac*M_PI)

	//
	// kick angles and color blend effects
	//

	cg_kickangles_t kickangles[MAX_ANGLES_KICKS];
	int64_t fallEffectTime;
	int64_t fallEffectRebounceTime;

	// awards
	char award_lines[MAX_AWARD_LINES][MAX_CONFIGSTRING_CHARS];
	int64_t award_times[MAX_AWARD_LINES];
	int award_head;

	// statusbar program
	struct cg_layoutnode_s *statusBar;

	cg_viewweapon_t weapon;
	cg_viewdef_t view;
};

extern cg_static_t cgs;
extern cg_state_t cg;

extern mempool_t *cg_mempool;

#define ISVIEWERENTITY( entNum )  ( cg.predictedPlayerState.POVnum > 0 && (int)cg.predictedPlayerState.POVnum == ( entNum ) && cg.view.type == VIEWDEF_PLAYERVIEW )

#define ISREALSPECTATOR()       ( cg.frame.playerState.real_team == TEAM_SPECTATOR )

extern centity_t cg_entities[MAX_EDICTS];

//
// cg_ents.c
//
bool CG_NewFrameSnap( snapshot_t *frame, snapshot_t *lerpframe );
struct cmodel_s *CG_CModelForEntity( int entNum );
void CG_SoundEntityNewState( centity_t *cent );
void CG_AddEntities( void );
void CG_GetEntitySpatilization( int entNum, Vec3 * origin, Vec3 * velocity );
void CG_LerpEntities( void );
void CG_LerpGenericEnt( centity_t *cent );
void CG_BBoxForEntityState( const SyncEntityState * state, Vec3 * mins, Vec3 * maxs );

//
// cg_draw.c
//
int CG_HorizontalAlignForWidth( int x, Alignment alignment, int width );
int CG_VerticalAlignForHeight( int y, Alignment alignment, int height );
Vec2 WorldToScreen( Vec3 v );
Vec2 WorldToScreenClamped( Vec3 v, Vec2 screen_border, bool * clamped );

//
// cg_media.c
//
void CG_RegisterMediaSounds( void );
void CG_RegisterMediaModels( void );
void CG_RegisterMediaShaders( void );
void CG_RegisterFonts( void );

//
// cg_players.c
//
extern cvar_t *cg_hand;

void CG_ResetClientInfos( void );
void CG_LoadClientInfo( int client );
void CG_RegisterPlayerSounds( PlayerModelMetadata * metadata, const char * name );
void CG_PlayerSound( int entnum, int entchannel, PlayerSound ps );

//
// cg_predict.c
//
extern cvar_t *cg_showMiss;

void CG_PredictedEvent( int entNum, int ev, u64 parm );
void CG_PredictedFireWeapon( int entNum, WeaponType weapon );
void CG_PredictMovement( void );
void CG_CheckPredictionError( void );
void CG_BuildSolidList( void );
void CG_Trace( trace_t *t, Vec3 start, Vec3 mins, Vec3 maxs, Vec3 end, int ignore, int contentmask );
int CG_PointContents( Vec3 point );
void CG_Predict_TouchTriggers( pmove_t *pm, Vec3 previous_origin );

//
// cg_screen.c
//
extern cvar_t *cg_showFPS;
extern cvar_t *cg_showAwards;

void CG_ScreenInit( void );
void CG_Draw2D( void );
void CG_CenterPrint( const char *str );

void CG_EscapeKey( void );

void CG_DrawCrosshair();
void CG_ScreenCrosshairDamageUpdate( void );

void CG_DrawKeyState( int x, int y, int w, int h, const char *key );

void CG_DrawClock( int x, int y, Alignment alignment, const Font * font, float font_size, Vec4 color, bool border );
void CG_DrawPlayerNames( const Font * font, float font_size, Vec4 color, bool border );
void CG_DrawNet( int x, int y, int w, int h, Alignment alignment, Vec4 color );

void CG_ClearPointedNum( void );

void CG_InitDamageNumbers();
void CG_AddDamageNumber( SyncEntityState * ent, u64 parm );
void CG_DrawDamageNumbers();

void CG_AddBomb( centity_t * cent );
void CG_AddBombSite( centity_t * cent );
void CG_DrawBombHUD();
void CG_ResetBombHUD();

void AddDamageEffect( float x = 0.0f );

//
// cg_hud.c
//
void CG_InitHUD();
void CG_ShutdownHUD();
void CG_SC_ResetObituaries();
void CG_SC_Obituary();
void CG_ExecuteLayoutProgram( struct cg_layoutnode_s *rootnode );
void CG_ClearAwards();

//
// cg_scoreboard.c
//
void CG_DrawScoreboard();
void CG_ScoresOn_f();
void CG_ScoresOff_f();
void SCR_UpdateScoreboardMessage( const char * string );
bool CG_ScoreboardShown();

//
// cg_main.c
//
extern cvar_t *developer;
extern cvar_t *cg_showClamp;
extern cvar_t *cg_showHotkeys;

// wsw
extern cvar_t *cg_volume_hitsound;    // hit sound volume
extern cvar_t *cg_volume_announcer; // announcer sounds volume
extern cvar_t *cg_autoaction_demo;
extern cvar_t *cg_autoaction_screenshot;
extern cvar_t *cg_autoaction_spectator;

extern cvar_t *cg_voiceChats;
extern cvar_t *cg_projectileAntilagOffset;
extern cvar_t *cg_chatFilter;

extern cvar_t *cg_allyModel;
extern cvar_t *cg_enemyModel;

extern cvar_t *cg_particleDebug;

#define CG_Malloc( size ) _Mem_AllocExt( cg_mempool, size, 16, 1, 0, 0, __FILE__, __LINE__ );
#define CG_Free( data ) Mem_Free( data )

void CG_Init( const char *serverName, unsigned int playerNum,
			  bool demoplaying, const char *demoName, unsigned snapFrameTime );
void CG_Shutdown( void );

#ifndef _MSC_VER
void CG_LocalPrint( const char *format, ... ) __attribute__( ( format( printf, 1, 2 ) ) );
#else
void CG_LocalPrint( _Printf_format_string_ const char *format, ... );
#endif

void CG_Reset( void );
void CG_Precache( void );
char *_CG_CopyString( const char *in, const char *filename, int fileline );
#define CG_CopyString( in ) _CG_CopyString( in, __FILE__, __LINE__ )

void CG_RegisterCGameCommands( void );
void CG_UnregisterCGameCommands( void );
void CG_AddAward( const char *str );

void CG_StartBackgroundTrack( void );

//
// cg_svcmds.c
//
void CG_ConfigString( int i, const char *s );
void CG_GameCommand( const char *command );
void CG_SC_AutoRecordAction( const char *action );

//
// cg_teams.c
//
void CG_RegisterPlayerModels();
const PlayerModelMetadata * CG_PModelForCentity( centity_t * cent );
RGB8 CG_TeamColor( int team );
Vec4 CG_TeamColorVec4( int team );

//
// cg_view.c
//
enum {
	CAM_INEYES,
	CAM_THIRDPERSON,
	CAM_MODES
};

struct ChasecamState {
	int mode;
	bool key_pressed;
};

extern ChasecamState chaseCam;

extern cvar_t *cg_thirdPerson;
extern cvar_t *cg_thirdPersonAngle;
extern cvar_t *cg_thirdPersonRange;

void CG_ResetKickAngles( void );

void CG_AddEntityToScene( entity_t *ent );
void CG_StartFallKickEffect( int bounceTime );
void CG_ViewSmoothPredictedSteps( Vec3 * vieworg );
float CG_ViewSmoothFallKick( void );
float CG_CalcViewFov();
void CG_RenderView( unsigned extrapolationTime );
Vec3 CG_GetKickAngles();
bool CG_ChaseStep( int step );
bool CG_SwitchChaseCamMode( void );

//
// cg_lents.c
//

void CG_BubbleTrail( Vec3 start, Vec3 end, int dist );
void CG_ProjectileTrail( const centity_t * cent );
void CG_RifleBulletTrail( const centity_t * cent );
void CG_NewBloodTrail( centity_t *cent );
void CG_BloodDamageEffect( Vec3 origin, Vec3 dir, int damage, Vec4 team_color );
void CG_PlasmaExplosion( Vec3 pos, Vec3 dir, Vec4 team_color );
void CG_BubbleExplosion( Vec3 pos, Vec4 team_color );
void CG_GrenadeExplosion( Vec3 pos, Vec3 dir, Vec4 team_color );
void CG_GenericExplosion( Vec3 pos, Vec3 dir, float radius );
void CG_RocketExplosion( Vec3 pos, Vec3 dir, Vec4 team_color );
void CG_EBBeam( Vec3 start, Vec3 end, Vec4 team_color );
void CG_EBImpact( Vec3 pos, Vec3 dir, int surfFlags, Vec4 team_color );
void CG_BladeImpact( Vec3 pos, Vec3 dir );
void CG_PModel_SpawnTeleportEffect( centity_t * cent, MatrixPalettes temp_pose );

void CG_Dash( const SyncEntityState *state );
void CG_DustCircle( Vec3 pos, Vec3 dir, float radius, int count );

void InitGibs();
void SpawnGibs( Vec3 origin, Vec3 velocity, int damage, Vec4 team_color );
void DrawGibs();

//
// cg_effects.c
//
void ExplosionParticles( Vec3 origin, Vec3 normal, Vec3 team_color );
void PlasmaImpactParticles( Vec3 origin, Vec3 normal, Vec3 team_color );
void BubbleImpactParticles( Vec3 origin, Vec3 team_color );
void RailTrailParticles( Vec3 start, Vec3 end, Vec4 color );

void DrawBeam( Vec3 start, Vec3 end, float width, Vec4 color, const Material * material );

void InitPersistentBeams();
void AddPersistentBeam( Vec3 start, Vec3 end, float width, Vec4 color, const Material * material, float duration, float fade_time );
void DrawPersistentBeams();

//
//	cg_vweap.c - client weapon
//
void CG_AddViewWeapon( cg_viewweapon_t *viewweapon );
void CG_CalcViewWeapon( cg_viewweapon_t *viewweapon );
void CG_ViewWeapon_StartAnimationEvent( int newAnim );

void CG_AddRecoil( WeaponType weapon );
void CG_Recoil( WeaponType weapon );

//
// cg_events.c
//
void CG_FireEvents( bool early );
void CG_EntityEvent( SyncEntityState *ent, int ev, u64 parm, bool predicted );
void CG_AddAnnouncerEvent( const SoundEffect *sound, bool queued );
void CG_ReleaseAnnouncerEvents( void );
void CG_ClearAnnouncerEvents( void );

// I don't know where to put these ones
void CG_WeaponBeamEffect( centity_t *cent );
void CG_LaserBeamEffect( centity_t *cent );


//
// cg_chat.cpp
//
void CG_InitChat();
void CG_ShutdownChat();
void CG_AddChat( const char * str );
void CG_DrawChat();

//
// cg_input.cpp
//

void CG_InitInput( void );
void CG_ShutdownInput( void );
void CG_ClearInputState( void );
void CG_MouseMove( int frameTime, Vec2 m );
float CG_GetSensitivityScale( float sens, float zoomSens );
unsigned int CG_GetButtonBits( void );
Vec3 CG_GetDeltaViewAngles();
Vec3 CG_GetMovement();

/*
* Returns angular movement vector (in euler angles) obtained from the input.
* Doesn't take flipping into account.
*/
void CG_GetAngularMovement( Vec3 movement );

bool CG_GetBoundKeysString( const char *cmd, char *keys, size_t keysSize );
int CG_GetBoundKeycodes( const char *cmd, int keys[ 2 ] );

/**
 * Checks a chat message for local player nick and flashes window on a match
 */
void CG_FlashChatHighlight( const unsigned int from, const char *text );
