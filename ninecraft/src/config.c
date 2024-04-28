
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <ninecraft/config.h>

ninecraft_cfg_option_t opt_INV_NUMBERS = {
	"use-numbers-to-navigate-inv",
	1
};
ninecraft_cfg_option_t opt_DROP_SLOT = {
	"drop-slot-using-q",
	1
};
ninecraft_cfg_option_t opt_BETTER_CREATIVE_CONTROLS = {
	"better-creative-controls",
	1
};

ninecraft_cfg_option_t opt_SCALING_FIX = {
	"scaling-fix",
	0
};

void writeBoolOptionTo(FILE* fp, ninecraft_cfg_option_t* option){
	int strsize = strlen(option->name);
	if(strsize >= 255){
		printf("the option name is too big(%s)!\n", option->name);
		return;
	}
	fwrite(option->name, sizeof(char), strsize, fp);
	fputc('=', fp);
	fputc(option->value.asbool ? '1' : '0', fp);
	fputc('\n', fp);
}

void writeConfigFile(){
	FILE* file = fopen(CONFIG_FILENAME, "w+");
	
	writeBoolOptionTo(file, &opt_INV_NUMBERS);
	writeBoolOptionTo(file, &opt_DROP_SLOT);
	writeBoolOptionTo(file, &opt_BETTER_CREATIVE_CONTROLS);
	writeBoolOptionTo(file, &opt_SCALING_FIX);
	
	fclose(file);
}

void setBool(ninecraft_cfg_option_t* option, char* value){
	option->value.asbool = value[0] == '0' ? 0 : 1;
}

void handleOption(char* name, char* value){
	printf("%s %s\n", name, value);
	
	if(strcmp(name, opt_INV_NUMBERS.name) == 0){
		setBool(&opt_INV_NUMBERS, value);
	}else if(strcmp(name, opt_DROP_SLOT.name) == 0){
		setBool(&opt_DROP_SLOT, value);
	}else if(strcmp(name, opt_BETTER_CREATIVE_CONTROLS.name) == 0){
		setBool(&opt_BETTER_CREATIVE_CONTROLS, value);
	}else if(strcmp(name, opt_SCALING_FIX.name) == 0){
		setBool(&opt_SCALING_FIX, value);
	}
	
}

void readConfigFile(){
	struct stat st;
	if(stat(CONFIG_FILENAME, &st) == 0){
		FILE* f = fopen(CONFIG_FILENAME, "r");
		int sind = 0;
		char c;
		char reading = 0; //0 file 1 value
		char str[256] = {0};
		char value[256] = {0};
		while((c = (char)fgetc(f)) != EOF){
			if(reading == 0){
				if(sind > 255){
					sind = 0; //not good way but whatever	
				}
				if(c == '\n'){
					printf("Failed to read option: %s\n", str);
					continue;
				}
				if(c == '='){
					str[sind] = 0;
					sind = 0;
					reading = 1;
				}else{
					str[sind] = c;
					++sind;
				}
			}else if(reading == 1){
				if(sind > 255){
					sind = 0; //not good way but whatever	
				}
				
				if(c == '\n'){
					value[sind] = 0;
					sind = 0;
					handleOption(str, value);
					reading = 0;
				}else{
					value[sind] = c;
					++sind;
				}
			}
		}
		fclose(f);
	}
	
	writeConfigFile();
}
