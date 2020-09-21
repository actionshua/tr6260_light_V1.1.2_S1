#ifndef YA_STRIPLIGHTS_APP_H_
#define YA_STRIPLIGHTS_APP_H_
#include "ya_common.h"

 typedef enum
 {
	IR_TYPE = 0,
	BUTTON_TYPE,
	CLOUD_TYPE,
	CONFIG_TYPE,
 }ya_striplight_control_msg_type_t;


typedef enum
{
	SCENE_READING,
	SCENE_WORK,
	SCENE_GOOD_NIGHT,
	SCENE_LEISURE,
	
	SCENE_DINNER,
	SCENE_DATE,
	SCENE_PARTY,
	SCENE_CINEMA,
	
	SCENE_SOFT_LIGHT,
	SCENE_PROFUSION,
	SCENE_DAZZLE_COLOUR,
	SCENE_COLORS,

	SCENE_MAX,
 } en_stripLigths_SCENE;

typedef enum
{
	COLORTYPE_WHITE,
	COLORTYPE_COLOR,
 } en_stripLigths_colorType;

typedef enum
{
	WORKMODE_WHITE = 0,
	WORKMODE_COLOR,
	WORKMODE_SCENE,
	WORKMODE_MUSIC,

	WORKMODE_MAX,
}en_stripLigths_work_mode;

typedef enum
{
	MUSICMODE_MADDEN = 0,
	MUSICMODE_ROCK,
	MUSICMODE_CLASSIC,
	MUSICMODE_SOFT,
	
	MUSICMODE_MAX,
}en_stripLigths_music;

typedef enum
{
	CTRLCMD_DYNAMICALLY_NOTSAVE,
	CTRLCMD_DYNAMICALLY_SAVE,
	CTRLCMD_STATE,
	CTRLCMD_RESTORE_FACTORY,
} en_stripLigths_CMD;
	



extern ya_hal_os_queue_t ya_striplight_queue;

//clear all scene data
extern int32_t ya_stripLights_app_sceneClearAll(void);

//button to queue
extern void ya_stripLights_app_button_event_into_queue(uint8_t msg_event);

//cloud event to queue
extern void ya_stripLights_app_cloud_event_into_queue(uint8_t *buf, uint16_t len);

//get update flag
extern bool ya_get_updata_stripLights(void);

//clear update flag
extern void ya_clear_updata_stripLights(void);

//report state to cloud
void ya_stripLights_cloud_attriReport(void);

void ya_stripLights_app_factory_test(void);


#endif

