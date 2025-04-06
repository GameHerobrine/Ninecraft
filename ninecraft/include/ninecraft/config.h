#ifndef CONFIG_H__
#define CONFIG_H__

#define CONFIG_FILENAME "config.cfg"

typedef struct {
	char* name;
	union{
		char asbool;
	} value;
} ninecraft_cfg_option_t;

void readConfigFile();

extern ninecraft_cfg_option_t opt_INV_NUMBERS;
extern ninecraft_cfg_option_t opt_DROP_SLOT;
extern ninecraft_cfg_option_t opt_BETTER_CREATIVE_CONTROLS;
extern ninecraft_cfg_option_t opt_TOGGLE_SHIFT;

extern ninecraft_cfg_option_t opt_SCALING_FIX;
extern ninecraft_cfg_option_t opt_NO_KEYBOARD_SPACE_IN_CHAT;

#endif
