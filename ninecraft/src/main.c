#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <sys/timeb.h>
#include <sys/stat.h>
#include <sys/types.h>
#ifndef _WIN32
#include <sys/mman.h>
#include <fnmatch.h>
#else
#include <direct.h>
#define mkdir(x, y) _mkdir(x)
#endif
#include <sys/types.h>
#include <sys/stat.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <ninecraft/patch/detours.h>
#include <ninecraft/gfx/gles_compat.h>
#include <ninecraft/version_ids.h>
#include <ninecraft/input/minecraft_keys.h>
#include <ninecraft/android/android_string.h>
#include <ninecraft/android/android_alloc.h>
#include <ninecraft/symbols.h>
#include <ninecraft/AppPlatform_linux.h>
#include <ninecraft/hooks.h>
#include <ninecraft/minecraft.h>
#include <ninecraft/input/keyboard.h>
#include <ninecraft/input/mouse_device.h>
#include <ninecraft/input/multitouch.h>
#include <ninecraft/mods/inject.h>
#include <ninecraft/mods/mod_loader.h>
#include <ninecraft/app_platform.h>
#include <math.h>
#include <wchar.h>
#include <wctype.h>
#include <ninecraft/audio/sles.h>
#include <ninecraft/audio/audio_engine.h>
#include <zlib.h>

#include <ninecraft/dlfcn_stub.h>
#include <ancmp/hooks.h>
#include <ancmp/android_dlfcn.h>
#include <ancmp/linker.h>

#include <ninecraft/math_compat.h>
#include <ninecraft/ninecraft_defs.h>

#include <ninecraft/options.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <ninecraft/config.h>
#include "cpputils.h"

void *handle = NULL;
GLFWwindow *_window = NULL;

int default_mouse_mode = GLFW_CURSOR_NORMAL;

int version_id = 0;

float y_cam = 0.0;
float x_cam = 0.0;

void *ninecraft_app;
AppPlatform_linux platform;

static unsigned char *controller_states;
static float *controller_x_stick;
static float *controller_y_stick;

bool mouse_pointer_hidden = false;

int old_pos_x, old_pos_y, old_width, old_height;


void *load_library(const char *name) {
#if defined(__i386__) || defined(_M_IX86)
    char *arch = "x86";
#else
#if defined(__arm__) || defined(_M_ARM) 
    char *arch = "armeabi-v7a";
#else
    char *arch = "";
#endif
#endif
    char *fullpath = (char *)malloc(1024);
    fullpath[0] = '\0';
    getcwd(fullpath, 1024);
    strcat(fullpath, "/lib/");
    strcat(fullpath, arch);
    strcat(fullpath, "/");
    strcat(fullpath, name);

    void *handle = internal_dlopen(fullpath, ANDROID_RTLD_LAZY);
    if (handle == NULL) {
        printf("failed to load library %s: %s\n", fullpath, internal_dlerror());
        return NULL;
    }
    printf("lib: %s: : %p\n", fullpath, handle);
    free(fullpath);
    return handle;
}

void stub_symbols(const char **symbols, void *stub_func) {
    int i = 0;
    while (true) {
        const char *sym = symbols[i];
        if (sym == NULL) {
            break;
        }
        add_custom_hook((char *)sym, stub_func);
        ++i;
    }
}

void __android_log_print(int prio, const char *tag, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    printf("[%s] ", tag);
    vprintf(fmt, args);
    printf("\n");
    va_end(args);
}

void android_stub() {
    puts("warn: android call");
}

void egl_stub() {
    // puts("warn: egl call");
}

int mouseToGameKeyCode(int keyCode) {
	if (keyCode == GLFW_MOUSE_BUTTON_LEFT) {
		return MCKEY_BREAK;
	} else if (keyCode == GLFW_MOUSE_BUTTON_RIGHT) {
		return MCKEY_PLACE;
	}
	return 0;
}
bool additionalBuildChecks = 0;
unsigned int lastBuiltTick = 0;

static void mouse_click_callback(GLFWwindow *window, int button, int action, int mods) {
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    if (!mouse_pointer_hidden) {
        int mc_button = (button == GLFW_MOUSE_BUTTON_LEFT ? 1 : (button == GLFW_MOUSE_BUTTON_RIGHT ? 2 : 0));
        if (version_id == version_id_0_1_0) {
            ((void (*)(int, int, int, int))internal_dlsym(handle, "_ZN5Mouse4feedEiiii"))((int)mc_button, (int)(action == GLFW_PRESS ? 1 : 0), (int)xpos, (int)ypos);
        } else if (version_id >= version_id_0_6_0) {
            mouse_device_feed_0_6(internal_dlsym(handle, "_ZN5Mouse9_instanceE"), (char)mc_button, (char)(action == GLFW_PRESS ? 1 : 0), (short)xpos, (short)ypos, 0, 0);
            multitouch_feed_0_6((char)mc_button, (char)(action == GLFW_PRESS ? 1 : 0), (short)xpos, (short)ypos, 0);
        } else if (version_id <= version_id_0_5_0_j) {
            mouse_device_feed_0_5(internal_dlsym(handle, "_ZN5Mouse9_instanceE"), (char)mc_button, (char)(action == GLFW_PRESS ? 1 : 0), (short)xpos, (short)ypos);
            multitouch_feed_0_5((char)mc_button, (char)(action == GLFW_PRESS ? 1 : 0), (short)xpos, (short)ypos, 0);
        }
    } else {
        int game_keycode = mouseToGameKeyCode(button);


        if(version_id == version_id_0_8_1 && game_keycode == MCKEY_PLACE){ //fix usage of bows, egg and snowball items
        	if(action == GLFW_PRESS) {
        		additionalBuildChecks = 1;
        		int placeActionTimeout = *(int*)((int)ninecraft_app + 3332);
        		if(!placeActionTimeout) lastBuiltTick = *(unsigned int*)(ninecraft_app + 3336);
        	}
        	else if(action == GLFW_RELEASE) additionalBuildChecks = 0;
        }
        
        if (action == GLFW_PRESS && !keyboard_states[game_keycode]) {
            keyboard_feed(game_keycode, 1);
        } else if (action == GLFW_RELEASE && keyboard_states[game_keycode]) {
            keyboard_feed(game_keycode, 0);
        }
    }
}

static void mouse_scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    char key_code = 0;
    if (yoffset > 0) {
        key_code = MCKEY_HOTBAR_PREVIOUS;
        if(mouse_pointer_hidden && version_id == version_id_0_8_1){
			int player = *(int*)(((int)ninecraft_app) + 3168);
			int inv = *(int*)(player + 3244);
			int slotSelected = *(int*)(inv + 40);
			if(slotSelected <= 0){
				void* fn = internal_dlsym(handle, "_ZN9Inventory10selectSlotEi");
				((void (*) (int, int)) fn)(inv, 7);
				return;
			}
        }
    } else if (yoffset < 0) {
        key_code = MCKEY_HOTBAR_NEXT;
        if(mouse_pointer_hidden && version_id == version_id_0_8_1){
			int player = *(int*)(((int)ninecraft_app) + 3168);
			int inv = *(int*)(player + 3244);
			int slotSelected = *(int*)(inv + 40);
			if(slotSelected >= 7){
				void* fn = internal_dlsym(handle, "_ZN9Inventory10selectSlotEi");
				((void (*) (int, int)) fn)(inv, 0);
				return;
			}
        }
    }
    
    keyboard_feed(key_code, 1);
    keyboard_feed(key_code, 0);
}



static double last_mouse_x = 0;
static double last_mouse_y = 0;
static bool ignore_relative_motion = false;
static void mouse_pos_callback(GLFWwindow *window, double xpos, double ypos) {
    if (!mouse_pointer_hidden || version_id >= version_id_0_6_0) {
        if (version_id == version_id_0_1_0) {
            ((void (*)(int, int, int, int))internal_dlsym(handle, "_ZN5Mouse4feedEiiii"))(0, 0, (int)xpos, (int)ypos);
        } else if (version_id >= version_id_0_6_0) {
            mouse_device_feed_0_6(internal_dlsym(handle, "_ZN5Mouse9_instanceE"), 0, 0, (short)xpos, (short)ypos, (short)(!ignore_relative_motion ? (xpos - last_mouse_x) : 0), (short)(!ignore_relative_motion ? (ypos - last_mouse_y) : 0));
            ignore_relative_motion = false;
            last_mouse_x = xpos;
            last_mouse_y = ypos;
            multitouch_feed_0_6(0, 0, (short)xpos, (short)ypos, 0);
        } else if (version_id <= version_id_0_5_0_j) {
            mouse_device_feed_0_5(internal_dlsym(handle, "_ZN5Mouse9_instanceE"), 0, 0, (short)xpos, (short)ypos);
            multitouch_feed_0_5(0, 0, (short)xpos, (short)ypos, 0);
        }
    } else {
        int cx;
        int cy;
	float scale = *(float*) internal_dlsym(handle, "_ZN3Gui11InvGuiScaleE");
        glfwGetWindowSize(window, &cx, &cy);
        cx *= scale;
        cy *= scale;
        if ((int)xpos != cy || (int)ypos != cy) {
            glfwSetCursorPos(window, cx, cy);
            y_cam -= ((float)ypos - (float)cy) / 1.7;
            x_cam += ((float)xpos - (float)cx) / 0.7;
        }
    }
}

int getGameKeyCode(int keycode) {
    if (keycode == GLFW_KEY_W) {
        return MCKEY_FORWARD;
    } else if (keycode == GLFW_KEY_A) {
        return MCKEY_STEP_LEFT;
    } else if (keycode == GLFW_KEY_S) {
        return MCKEY_BACKWARDS;
    } else if (keycode == GLFW_KEY_D) {
        return MCKEY_STEP_RIGHT;
    } else if (keycode == GLFW_KEY_SPACE) {
        return MCKEY_JUMP;
    } else if (keycode == GLFW_KEY_E) {
        return MCKEY_INVENTORY;
    } else if (keycode == GLFW_KEY_ESCAPE) {
        return MCKEY_PAUSE;
    } else if (keycode == GLFW_KEY_C) {
        return MCKEY_CRAFTING;
    } else if (keycode == GLFW_KEY_ENTER) {
        return MCKEY_SIGN_ENTER;
    } else if (keycode == GLFW_KEY_BACKSPACE) {
        return MCKEY_SIGN_BACKSPACE;
    } else if (keycode == GLFW_KEY_LEFT_CONTROL) {
        return MCKEY_MENU_CANCEL;
    } else {
        return 0;
    }
}

void set_ninecraft_size_0_1_0(int width, int height) {
    float *inv_gui_scale = (float *)internal_dlsym(handle, "_ZN3Gui11InvGuiScaleE");
    int *mc_width = (int *)internal_dlsym(handle, "_ZN9Minecraft5widthE");
    int *mc_height = (int *)internal_dlsym(handle, "_ZN9Minecraft6heightE");
    void (*screen_set_size)(void *screen, int width, int height) = (void (*)(void *screen, int width, int height))internal_dlsym(handle, "_ZN6Screen7setSizeEii");

    *mc_width = width;
    *mc_height = height;
    
    if (width < 1000) {
        if (width < 800) {
            if (width < 400) {
                *inv_gui_scale = 1.0;
            } else {
                *inv_gui_scale = 0.5;
            }
        } else {
            *inv_gui_scale = 0.3333333;
        }
    } else {
        *inv_gui_scale = 0.25;
    }
    void *screen = *(void **)((char *)ninecraft_app + 0xd14);
    if (screen != NULL) {
        float new_screen_width = (float)(width) * (*inv_gui_scale);
        float new_screen_height = (float)(height) * (*inv_gui_scale);
        screen_set_size(
            screen,
            (int)(0.0 < new_screen_width) * (int)new_screen_width,
            (int)(0.0 < new_screen_height) * (int)new_screen_height);
        ((int *)screen)[6] = 0;
        ((int *)screen)[7] = 0;
        ((int *)screen)[8] = 0;
        ((int *)screen)[9] = 0;
        ((int *)screen)[10] = 0;
        ((int *)screen)[11] = 0;
        ((void (*)(void *))((*(int **)screen)[3]))(screen);
        

    }
}

static void set_ninecraft_size(int width, int height) {
    if (version_id >= version_id_0_10_0) {
        minecraft_client_set_size(ninecraft_app, width, height, 2.f);
    } else {
        glClear(GL_DEPTH_BUFFER_BIT);
        minecraft_set_size(ninecraft_app, width, height);
        // Scaling fix
        if(opt_SCALING_FIX.value.asbool){
        	size_t screen_offset;
			if (version_id == version_id_0_8_1) {
				screen_offset = NINECRAFTAPP_SCREEN_OFFSET_0_8_1;
			} else if (version_id == version_id_0_8_0) {
				screen_offset = NINECRAFTAPP_SCREEN_OFFSET_0_8_0;
			} else if (version_id == version_id_0_7_6) {
				screen_offset = NINECRAFTAPP_SCREEN_OFFSET_0_7_6;
			} else if (version_id == version_id_0_7_5) {
				screen_offset = NINECRAFTAPP_SCREEN_OFFSET_0_7_5;
			} else if (version_id == version_id_0_7_4) {
				screen_offset = NINECRAFTAPP_SCREEN_OFFSET_0_7_4;
			} else if (version_id == version_id_0_7_3) {
				screen_offset = NINECRAFTAPP_SCREEN_OFFSET_0_7_3;
			} else if (version_id == version_id_0_7_2) {
				screen_offset = NINECRAFTAPP_SCREEN_OFFSET_0_7_2;
			} else if (version_id == version_id_0_7_1) {
				screen_offset = NINECRAFTAPP_SCREEN_OFFSET_0_7_1;
			} else if (version_id == version_id_0_7_0) {
				screen_offset = NINECRAFTAPP_SCREEN_OFFSET_0_7_0;
			} else {
				return;
			}
			*(float *)internal_dlsym(handle, "_ZN3Gui11InvGuiScaleE") = 0.5f;
			*(float *)internal_dlsym(handle, "_ZN3Gui8GuiScaleE") = 2.0f;
			void *screen = *(void **)(ninecraft_app + screen_offset);
			if (screen) {
				((void (*)(void *, int, int))internal_dlsym(handle, "_ZN6Screen7setSizeEii"))(screen, width * 0.5f, height * 0.5f);
			}
        }
       
    }
}

static void resize_callback(GLFWwindow *window, int width, int height) {
    if (version_id == version_id_0_1_0) {
        set_ninecraft_size_0_1_0(width, height);
    } else {
        set_ninecraft_size(width, height);
    }
}
bool chatJustOpened = 0;
static void char_callback(GLFWwindow *window, unsigned int codepoint) {
    if (is_keyboard_visible) {
    	if(chatJustOpened){
    		chatJustOpened = false;
    		return;
    	}
        if (version_id >= version_id_0_6_0 && version_id <= version_id_0_7_1) {
            keyboard_feed_text_0_6_0((char)codepoint);
        } else if (version_id >= version_id_0_7_2) {
            // char p_codepoint[2] = {(char)codepoint, '\0'};
            char p_codepoint[5];

            if (codepoint <= 0x7f) {
                p_codepoint[0] = (char)codepoint;
                p_codepoint[1] = '\x00';
            } else if (codepoint <= 0x7fff) {
                p_codepoint[0] = (char)(0xc0 | ((codepoint >> 6) & 0x1f));
                p_codepoint[1] = (char)(0x80 | ((codepoint & 0x3f)));
                p_codepoint[2] = '\x00';
            } else if (codepoint <= 0xffff) {
                p_codepoint[0] = (char)(0xe0 | ((codepoint >> 12) & 0x0f));
                p_codepoint[1] = (char)(0x80 | ((codepoint >> 6) & 0x3f));
                p_codepoint[2] = (char)(0x80 | (codepoint & 0x3f));
                p_codepoint[3] = '\x00';
            } else {
                p_codepoint[0] = (char)(0xf0 | ((codepoint >> 18) & 0x07));
                p_codepoint[1] = (char)(0x80 | ((codepoint >> 12) & 0x3f));
                p_codepoint[2] = (char)(0x80 | ((codepoint >> 6) & 0x3f));
                p_codepoint[3] = (char)(0x80 | (codepoint & 0x3f));
                p_codepoint[4] = '\x00';
            }

            android_string_t str;
            android_string_cstr((android_string_t *)&str, p_codepoint);
            keyboard_feed_text_0_7_2(&str, false);
        }
    }
}

bool flyDown, flyUp;
bool sneaking_095 = 0;
char keyF5 = 0;
static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_F11) {
        if (action == GLFW_PRESS) {
            if (glfwGetWindowMonitor(_window) == NULL) {
                glfwGetWindowPos(_window, &old_pos_x, &old_pos_y);
                glfwGetWindowSize(_window, &old_width, &old_height);
                const GLFWvidmode *mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
                glfwSetWindowMonitor(_window, glfwGetPrimaryMonitor(), 0, 0, mode->width, mode->height, 0);
            } else {
                glfwSetWindowMonitor(_window, NULL, old_pos_x, old_pos_y, old_width, old_height, 0);
            }
        }
    } else {

        int game_keycode = getGameKeyCode(key);
        
        if(mouse_pointer_hidden){
		int off_mc_thePlayer = 0;
		int off_player_inventory = 0;
		int off_inventory_currentSlot = 0;
		int off_player_abilities = 0;

		if(version_id == version_id_0_8_1){
			off_mc_thePlayer = 3168;
			off_player_inventory = 3244;
			off_inventory_currentSlot = 40;
			off_player_abilities = 3208;
		}else if(version_id == version_id_0_7_6){
			off_mc_thePlayer = 3128;
			off_player_inventory = 3120;
			off_inventory_currentSlot = 36;
			off_player_abilities = 3084;
		}else if(version_id == version_id_0_9_5){
			off_mc_thePlayer = 797*4;
			off_player_inventory = 3208;
			off_inventory_currentSlot = 40;
		}else{
			goto BC_NOT_IMPL;
		}

		
		int player = *(int*)(((int)ninecraft_app) + off_mc_thePlayer);
		int inv = *(int*)(player + off_player_inventory);

		if(key >= GLFW_KEY_1 && key <= GLFW_KEY_8 && opt_INV_NUMBERS.value.asbool){
			int slot = 8 - (GLFW_KEY_8 - key + 1);
			void* fn = internal_dlsym(handle, "_ZN9Inventory10selectSlotEi");
			((void (*) (int, int)) fn)(inv, slot);
			return;
	        }
	        
		if(key == GLFW_KEY_Q && opt_DROP_SLOT.value.asbool){ //drop using q
			int slot = *(int*)(inv+off_inventory_currentSlot);
			if(slot <= 8){
				void* fn = internal_dlsym(handle, "_ZN16FillingContainer8dropSlotEibb");
				((void (*) (int, int, char, char)) fn)(inv, slot, 0, 0);
			}
			return;
	        }
	        
		if((key == GLFW_KEY_SPACE || key == GLFW_KEY_LEFT_SHIFT) && opt_BETTER_CREATIVE_CONTROLS.value.asbool){ //better creative controls
			char isFlying = *(char*)(player + off_player_abilities + 1); //player->abilities.flying
	        	if(isFlying){
	        		if(key == GLFW_KEY_LEFT_SHIFT){ //avoid sneaking
	        			return;
	        		}
	        	}
		}
	BC_NOT_IMPL:
        }
        
        if(mouse_pointer_hidden && key == GLFW_KEY_F5){
        	char b = action != GLFW_RELEASE;
        	if(b != keyF5){
        		keyF5 = b;
        		if(b){
				int offset = 60;
				if(version_id == version_id_0_9_5) offset = 56;
				if(version_id == version_id_0_5_0) return; //offset = 40;

				void* options = (void*) ((int)ninecraft_app + offset);
				if(version_id == version_id_0_10_5) options = *(void**)((int)ninecraft_app + 252);

				void* third_person = internal_dlsym(handle, "_ZN7Options6Option12THIRD_PERSONE");
				void (*fn)(void*, void*, int) = internal_dlsym(handle, "_ZN7Options6toggleEPKNS_6OptionEi");
				fn(options, third_person, keyF5); //last arg doesnt matter here
        		}
        	}
        }
        
        if (mouse_pointer_hidden && key == GLFW_KEY_LEFT_SHIFT) {
        	if(!opt_TOGGLE_SHIFT.value.asbool){
			if(version_id == version_id_0_8_1){
	        		int player = *(int*)(((int)ninecraft_app) + 3168);
        			int moveinp = *(int*)(player + 3408);
        			*(char*)(moveinp + 14) = (action != GLFW_RELEASE);
			}else if(version_id == version_id_0_9_5){
				sneaking_095 = (action != GLFW_RELEASE); //0.9 overrides vanilla sneaking
			}else{
				goto lshift_fallback;
			}
        	}else{
			lshift_fallback:
			if(version_id == version_id_0_9_5){
				if(action == GLFW_PRESS) sneaking_095 = !sneaking_095;
			}else{
				if (action == GLFW_PRESS) {
	        			controller_states[0] = 1;
				} else if (action == GLFW_RELEASE) {
					controller_states[0] = 0;
				}
			}
        	}
        } else if (mouse_pointer_hidden && key == GLFW_KEY_T) {
            if (action == GLFW_PRESS) {
                size_t minecraft_screenchooser_offset;
		int screenid = 7;
		char useScreenChooser = 1;

                if (version_id == version_id_0_7_0) {
                    minecraft_screenchooser_offset = MINECRAFT_SCREENCHOOSER_OFFSET_0_7_0;
                } else if (version_id == version_id_0_7_1) {
                    minecraft_screenchooser_offset = MINECRAFT_SCREENCHOOSER_OFFSET_0_7_1;
                } else if (version_id == version_id_0_7_2) {
                    minecraft_screenchooser_offset = MINECRAFT_SCREENCHOOSER_OFFSET_0_7_2;
                } else if (version_id == version_id_0_7_3) {
                    minecraft_screenchooser_offset = MINECRAFT_SCREENCHOOSER_OFFSET_0_7_3;
                } else if (version_id == version_id_0_7_4) {
                    minecraft_screenchooser_offset = MINECRAFT_SCREENCHOOSER_OFFSET_0_7_4;
                } else if (version_id == version_id_0_7_5) {
                    minecraft_screenchooser_offset = MINECRAFT_SCREENCHOOSER_OFFSET_0_7_5;
                } else if (version_id == version_id_0_7_6) {
                    minecraft_screenchooser_offset = MINECRAFT_SCREENCHOOSER_OFFSET_0_7_6;
                } else if (version_id == version_id_0_8_0) {
                    minecraft_screenchooser_offset = MINECRAFT_SCREENCHOOSER_OFFSET_0_8_0;
                } else if (version_id == version_id_0_8_1) {
                    minecraft_screenchooser_offset = MINECRAFT_SCREENCHOOSER_OFFSET_0_8_1;
                } else if(version_id == version_id_0_9_5){
			minecraft_screenchooser_offset = 408;
			screenid = 5; //6 = start chat message with /
		} else if(version_id == version_id_0_10_5){
			useScreenChooser = 0;
			void* chat = malloc(0xf0);
			void (*constructor)(void*, int) = internal_dlsym(handle, "_ZN10ChatScreenC2Eb");
			constructor(chat, 0);
			((void (*) (void*, void*)) internal_dlsym(handle, "_ZN15MinecraftClient9setScreenEP6Screen"))(ninecraft_app, chat);
		}else {
			return;
		}


		if(useScreenChooser) ((void (*)(void *, int))internal_dlsym(handle, "_ZN13ScreenChooser9setScreenE8ScreenId"))(ninecraft_app + minecraft_screenchooser_offset, screenid);
		
                if(version_id == version_id_0_8_1){
                	chatJustOpened = 1;
                	int** guiScreen = *(int***)((int)ninecraft_app + 3184);
                	void* buttonClicked = (void*) guiScreen[0][30];
                	((void (*)(int**, int*)) buttonClicked)(guiScreen, guiScreen[24]);
                }else if(version_id == version_id_0_7_6){
			chatJustOpened = 1;
			int** guiScreen = *(int***)((int)ninecraft_app + 3144);
			void* buttonClicked = (void*) guiScreen[0][29];
			((void (*)(int**, int*)) buttonClicked)(guiScreen, guiScreen[24]);
		}else if(version_id == version_id_0_9_5){
			chatJustOpened = 1;
			int** guiScreen = *(int***)((int)ninecraft_app + 3200);
			void* buttonClicked = (void*) guiScreen[0][32];
			((void (*)(int**, int*)) buttonClicked)(guiScreen, guiScreen[36]);
		}else if(version_id == version_id_0_10_5){
			chatJustOpened = 1;
			int** guiScreen = *(int***)((int)ninecraft_app + 108);
			void* buttonClicked = (void*) guiScreen[0][32];
			((void (*)(int**, int*)) buttonClicked)(guiScreen, guiScreen[53]);
		}
            }
        } else if (version_id >= version_id_0_1_1 && key == GLFW_KEY_ESCAPE) {
            if (action == GLFW_PRESS) {
                if (version_id >= version_id_0_10_0) {
                    minecraft_client_handle_back(ninecraft_app, false);
                } else {
                    ninecraft_app_handle_back(ninecraft_app, false);
                }
            }
        } else if (action == GLFW_PRESS && game_keycode) {
            keyboard_feed(game_keycode, 1);
        } else if (action == GLFW_RELEASE && game_keycode) {
            keyboard_feed(game_keycode, 0);
        }
    }
}

void window_close_callback(GLFWwindow *window) {
	audio_engine_destroy();
	exit(0);
}

void grab_mouse() {
    puts("grab_mouse");
    mouse_pointer_hidden = true;
    ignore_relative_motion = true;
    glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    //glfwSetInputMode(_window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
}

void release_mouse() {
    puts("release_mouse");
    mouse_pointer_hidden = false;
    glfwSetInputMode(_window, GLFW_CURSOR, default_mouse_mode);
    //glfwSetInputMode(_window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    double cursor_x;
    double cursor_y;
    glfwGetCursorPos(_window, &cursor_x, &cursor_y);
    mouse_pos_callback(_window, cursor_x, cursor_y);
}

void gles_hook() {
    add_custom_hook("glAlphaFunc", (void *)gl_alpha_func);
    add_custom_hook("glBindBuffer", (void *)gl_bind_buffer);
    add_custom_hook("glBindTexture", (void *)gl_bind_texture);
    add_custom_hook("glBlendFunc", (void *)gl_blend_func);
    add_custom_hook("glBufferData", (void *)gl_buffer_data);
    add_custom_hook("glClear", (void *)gl_clear);
    add_custom_hook("glClearColor", (void *)gl_clear_color);
    add_custom_hook("glColor4f", (void *)gl_color_4_f);
    add_custom_hook("glColorMask", (void *)gl_color_mask);
    add_custom_hook("glColorPointer", (void *)gl_color_pointer);
    add_custom_hook("glCullFace", (void *)gl_cull_face);
    add_custom_hook("glDeleteBuffers", (void *)gl_delete_buffers);
    add_custom_hook("glDeleteTextures", (void *)gl_delete_textures);
    add_custom_hook("glDepthFunc", (void *)gl_depth_func);
    add_custom_hook("glDepthMask", (void *)gl_depth_mask);
    add_custom_hook("glDepthRangef", (void *)gl_depth_range_f);
    add_custom_hook("glDisable", (void *)gl_disable);
    add_custom_hook("glDisableClientState", (void *)gl_disable_client_state);
    add_custom_hook("glDrawArrays", (void *)gl_draw_arrays);
    add_custom_hook("glEnable", (void *)gl_enable);
    add_custom_hook("glEnableClientState", (void *)gl_enable_client_state);
    add_custom_hook("glFogf", (void *)gl_fog_f);
    add_custom_hook("glFogfv", (void *)gl_fog_f_v);
    add_custom_hook("glFogx", (void *)gl_fog_x);
    add_custom_hook("glGenTextures", (void *)gl_gen_textures);
    add_custom_hook("glGetFloatv", (void *)gl_get_float_v);
    add_custom_hook("glGetString", (void *)gl_get_string);
    add_custom_hook("glHint", (void *)gl_hint);
    add_custom_hook("glLineWidth", (void *)gl_line_width);
    add_custom_hook("glLoadIdentity", (void *)gl_load_identity);
    add_custom_hook("glMatrixMode", (void *)gl_matrix_mode);
    add_custom_hook("glMultMatrixf", (void *)gl_mult_matrix_f);
    add_custom_hook("glNormal3f", (void *)gl_normal_3_f);
    add_custom_hook("glOrthof", (void *)gl_ortho_f);
    add_custom_hook("glPolygonOffset", (void *)gl_polygon_offset);
    add_custom_hook("glPopMatrix", (void *)gl_pop_matrix);
    add_custom_hook("glPushMatrix", (void *)gl_push_matrix);
    add_custom_hook("glReadPixels", (void *)gl_read_pixels);
    add_custom_hook("glRotatef", (void *)gl_rotate_f);
    add_custom_hook("glScalef", (void *)gl_scale_f);
    add_custom_hook("glScissor", (void *)gl_scissor);
    add_custom_hook("glShadeModel", (void *)gl_shade_model);
    add_custom_hook("glTexCoordPointer", (void *)gl_tex_coord_pointer);
    add_custom_hook("glTexImage2D", (void *)gl_tex_image_2_d);
    add_custom_hook("glTexParameteri", (void *)gl_tex_parameter_i);
    add_custom_hook("glTexSubImage2D", (void *)gl_tex_sub_image_2_d);
    add_custom_hook("glTranslatef", (void *)gl_translate_f);
    add_custom_hook("glVertexPointer", (void *)gl_vertex_pointer);
    add_custom_hook("glViewport", (void *)gl_viewport);
    add_custom_hook("glDrawElements", (void *)gl_draw_elements);
    add_custom_hook("glGetError", (void *)gl_get_error);
    add_custom_hook("glGenBuffers", (void *)gl_gen_buffers);
    add_custom_hook("glStencilFunc", (void *)gl_stencil_func);
    add_custom_hook("glStencilMask", (void *)gl_stencil_mask);
    add_custom_hook("glLightModelf", (void *)gl_light_model_f);
    add_custom_hook("glLightfv", (void *)gl_light_f_v);
    add_custom_hook("glNormalPointer", (void *)gl_normal_pointer);
    add_custom_hook("glStencilOp", (void *)gl_stencil_op);
    add_custom_hook("glActiveTexture", (void *)gl_active_texture);
    add_custom_hook("glAttachShader", (void *)gl_attach_shader);
    add_custom_hook("glClearStencil", (void *)gl_clear_stencil);
    add_custom_hook("glCompileShader", (void *)gl_compile_shader);
    add_custom_hook("glCreateProgram", (void *)gl_create_program);
    add_custom_hook("glCreateShader", (void *)gl_create_shader);
    add_custom_hook("glDeleteProgram", (void *)gl_delete_program);
    add_custom_hook("glEnableVertexAttribArray", (void *)gl_enable_vertex_attrib_array);
    add_custom_hook("glGetActiveAttrib", (void *)gl_get_active_attrib);
    add_custom_hook("glGetActiveUniform", (void *)gl_get_active_uniform);
    add_custom_hook("glGetAttribLocation", (void *)gl_get_attrib_location);
    add_custom_hook("glGetProgramInfoLog", (void *)gl_get_program_info_log);
    add_custom_hook("glGetProgramiv", (void *)gl_get_program_i_v);
    add_custom_hook("glGetShaderInfoLog", (void *)gl_get_shader_info_log);
    add_custom_hook("glGetShaderiv", (void *)gl_get_shader_i_v);
    add_custom_hook("glGetShaderPrecisionFormat", (void *)gl_get_shader_precision_format);
    add_custom_hook("glGetUniformLocation", (void *)gl_get_uniform_location);
    add_custom_hook("glLinkProgram", (void *)gl_link_program);
    add_custom_hook("glReleaseShaderCompiler", (void *)gl_release_shader_compiler);
    add_custom_hook("glShaderSource", (void *)gl_shader_source);
    add_custom_hook("glUniform1fv", (void *)gl_uniform_1_f_v);
    add_custom_hook("glUniform1iv", (void *)gl_uniform_1_i_v);
    add_custom_hook("glUniform2fv", (void *)gl_uniform_2_f_v);
    add_custom_hook("glUniform2iv", (void *)gl_uniform_2_i_v);
    add_custom_hook("glUniform3fv", (void *)gl_uniform_3_f_v);
    add_custom_hook("glUniform3iv", (void *)gl_uniform_3_i_v);
    add_custom_hook("glUniform4fv", (void *)gl_uniform_4_f_v);
    add_custom_hook("glUniform4iv", (void *)gl_uniform_4_i_v);
    add_custom_hook("glUniformMatrix2fv", (void *)gl_uniform_matrix_2_f_v);
    add_custom_hook("glUniformMatrix3fv", (void *)gl_uniform_matrix_3_f_v);
    add_custom_hook("glUniformMatrix4fv", (void *)gl_uniform_matrix_4_f_v);
    add_custom_hook("glUseProgram", (void *)gl_use_program);
    add_custom_hook("glVertexAttribPointer", (void *)gl_vertex_attrib_pointer);
}

#ifdef __arm__

extern void __aeabi_uidiv();

extern void __aeabi_atexit();

extern void __aeabi_d2ulz();

extern void __aeabi_uidivmod();

extern void __aeabi_uldivmod();

extern void __aeabi_ldivmod();

extern void __aeabi_ul2d();

extern void __aeabi_idivmod();

extern void __aeabi_idiv();

extern void __aeabi_ul2f();

#endif

void __my_cxa_pure_virtual() {
    perror("Called none existant function");
    abort();
}

NINECRAFT_FLOAT_FUNC double io_strtod(const char *__nptr, char **__endptr) {
    return strtod(__nptr, __endptr);
}

void *cpp_operator_new(size_t size) {
    void *ptr = malloc(size);
    if (ptr == NULL) {
        perror("Failed to allocate memory");
        abort();
    }
    return ptr;
}

void cpp_operator_delete(void *ptr) {
    if (ptr) {
        free(ptr);
    }
}

extern void *__dso_handle;

#define CONSTRUCTION_NOT_YET_STARTED 0
#define CONSTRUCTION_COMPLETE 1
#define CONSTRUCTION_UNDERWAY_WITHOUT_WAITER 0x100
#define CONSTRUCTION_UNDERWAY_WITH_WAITER 0x200
/*
typedef union
{
    atomic_int state;
    int32_t aligner;
} my_guard_t;

int __my_cxa_guard_acquire(my_guard_t *gv) {
    int old_value = atomic_load_explicit(&gv->state, memory_order_relaxed);
    while (true) {
        if (old_value == CONSTRUCTION_COMPLETE) {
            atomic_thread_fence(memory_order_acquire);
            return 0;
        } else if (old_value == CONSTRUCTION_NOT_YET_STARTED) {
            if (!atomic_compare_exchange_weak_explicit(&gv->state, &old_value,
                                                       CONSTRUCTION_UNDERWAY_WITHOUT_WAITER,
                                                       memory_order_relaxed,
                                                       memory_order_relaxed)) {
                continue;
            }
            atomic_thread_fence(memory_order_acquire);
            return 1;
        } else if (old_value == CONSTRUCTION_UNDERWAY_WITHOUT_WAITER) {
            if (!atomic_compare_exchange_weak_explicit(&gv->state, &old_value,
                                                       CONSTRUCTION_UNDERWAY_WITH_WAITER,
                                                       memory_order_relaxed,
                                                       memory_order_relaxed)) {
                continue;
            }
        }
        //syscall(SYS_futex, &gv->state, FUTEX_WAIT, CONSTRUCTION_UNDERWAY_WITH_WAITER, NULL, NULL, 0);
        old_value = atomic_load_explicit(&gv->state, memory_order_relaxed);
    }
}

void __my_cxa_guard_release(my_guard_t *gv) {
    int old_value = atomic_exchange_explicit(&gv->state, CONSTRUCTION_COMPLETE, memory_order_release);
    if (old_value == CONSTRUCTION_UNDERWAY_WITH_WAITER) {
        //syscall(SYS_futex, &gv->state, FUTEX_WAKE, INT_MAX, NULL, NULL, 0);
    }
}*/

int __my_srget(FILE *astream) {
    puts("__srget");
    return EOF;
}

void db_test() {
    puts("db_test");
}

void missing_hook() {
    /*add_custom_hook("__cxa_guard_release", __my_cxa_guard_release);
    add_custom_hook("__cxa_guard_acquire", __my_cxa_guard_acquire);
    add_custom_hook("__dso_handle", &__dso_handle);
    add_custom_hook("fnmatch", fnmatch);
    add_custom_hook("__srget", __my_srget);*/
    
    /*add_custom_hook("_Znaj", cpp_operator_new);
    add_custom_hook("_ZdaPv", cpp_operator_delete);
    add_custom_hook("_Znwj", cpp_operator_new);
    add_custom_hook("_ZdlPv", cpp_operator_delete);*/
    

    add_custom_hook("deflateInit_", deflateInit_);
    add_custom_hook("deflateInit2_", deflateInit2_);
    add_custom_hook("deflate", deflate);
    add_custom_hook("deflateEnd", deflateEnd);
    add_custom_hook("inflateInit_", inflateInit_);
    add_custom_hook("inflateInit2_", inflateInit2_);
    add_custom_hook("inflate", inflate);
    add_custom_hook("inflateEnd", inflateEnd);
    add_custom_hook("uncompress", uncompress);
    add_custom_hook("compress", compress);
    add_custom_hook("compressBound", compressBound);

#ifdef __arm__
    add_custom_hook("__aeabi_atexit", __aeabi_atexit);
    add_custom_hook("__aeabi_uidiv", __aeabi_uidiv);
    add_custom_hook("__aeabi_d2ulz", __aeabi_d2ulz);
    add_custom_hook("__aeabi_uidivmod", __aeabi_uidivmod);
    add_custom_hook("__aeabi_uldivmod", __aeabi_uldivmod);
    add_custom_hook("__aeabi_ldivmod", __aeabi_ldivmod);
    add_custom_hook("__aeabi_ul2d", __aeabi_ul2d);
    add_custom_hook("__aeabi_idivmod", __aeabi_idivmod);
    add_custom_hook("__aeabi_idiv", __aeabi_idiv);
    add_custom_hook("__aeabi_ul2f", __aeabi_ul2f);
#endif
}

ninecraft_options_t options = {
    .options = NULL,
    .length = 0,
    .capasity = 0
};


bool LocalPlayer_isSneaking_hook_095(int this){
	return sneaking_095;
}

void (*ControllerMoveInput_tick_real_095)(int, int);
void ControllerMoveInput_tick_hook_095(int this, int player){
	bool flight = 0;
	bool tf = *(char*)(player + 3169);
	if(*(char*)(player + 3169) && *(char*)(this + 0xf + 7)){ //isFlying && space
		if(*(char*)(this + 0xf + 3) || *(char*)(this + 0xf + 4)){ //w || s
			// *(char*)(player + 3169) = 0; //player doesnt fly -> no vanilla checks
			// *(char*)(this+0xf+7) = 0;
			flight = 1;
		}
	}

	if(*(char*)(this + 0xf + 3)){ //w
		*(float*)(this+0x8) = 1;
	}else if(*(char*)(this + 0xf + 4)){ //s
		*(float*)(this+0x8) = -1;
	}
	*(char*)(this + 32) = 0; //needed to make forward work
	*(char*)(this + 33) = 0; //needed to make backward work
	ControllerMoveInput_tick_real_095(this, player);

	//for(int i = 0; i < 0xf+16; ++i) printf("%d ", *(char*)(this + i));
	//printf("\n");
	
	if(tf){
		*(char*)(this + 4 + 0xB) = glfwGetKey(_window, GLFW_KEY_SPACE) != GLFW_RELEASE;
		*(char*)(this + 4 + 0xC) = glfwGetKey(_window, GLFW_KEY_LEFT_SHIFT) != GLFW_RELEASE;
	}

	if(flight){
		*(char*)(player + 3169) = 1;
	}else{
		*(char*)(this + 4 + 0xB) = *(char*)(this + 0xf + 7) && *(char*)(player + 3169);
		if(*(char*)(this + 0xf + 3) && !tf) {
			*(char*)(this + 4 + 0xC) = 0;
		}
	}
}

void (*XperialPlayInput_tick_real_081)(int, int);
void XperialPlayInput_tick_hook_081(int this, int player){
	bool flight = false;

	if(*(char*)(player + 3209) && *(char*)(this + 0xf + 7)){ //isFlying && space
		if(*(char*)(this + 0xf + 3) || *(char*)(this + 0xf + 4)){ //w || s
			*(char*)(player + 3209) = 0; //player doesnt fly -> no vanilla checks
			flight = true;
		}
	}
	
	XperialPlayInput_tick_real_081(this, player);
	
	*(char*)(this + 4 + 0xB) = glfwGetKey(_window, GLFW_KEY_SPACE) != GLFW_RELEASE;
	*(char*)(this + 4 + 0xC) = glfwGetKey(_window, GLFW_KEY_LEFT_SHIFT) != GLFW_RELEASE;
	if(flight){
		*(char*)(player + 3209) = 1;
	}
}

void (*TextBox_keyPressed_real_081)(void* this, void* mc, int i);
char Minecraft_useTouchscreen_095(void** mc){
	void* screen = mc[800];
	if(screen) return 1;
	return 0;
}
char useTouchScreen_0105 = 0;
void (*MinecraftClient__reloadInput_0105_real)(void** mc);

/*char MinecraftClient_useController_0105(void** mc){
	return 1;
}*/

char MinecraftClient_useTouchscreen_0105(void** mc){
	void* screen = mc[27];
	char ret = 0;
	if(screen) {
		ret = 1;
	}
	if(useTouchScreen_0105 != ret){
		useTouchScreen_0105 = ret;
		MinecraftClient__reloadInput_0105_real(mc);
	}

	return ret;
}
int Gui_getNumSlots_095(void* gui){
	return 9;
}

int (*Gui_getNumSlots_0105)(void* gui) = Gui_getNumSlots_095;

#ifndef _WIN32
extern int __cxa_atexit(void (*)(void*), void*, void*);
#endif

int main(int argc, char **argv) {
    printf("sizeof wchar: %d\n", sizeof(wchar_t));
    android_linker_init();
    static struct stat st = {0};
    if (stat("storage", &st) == -1) {
        mkdir("storage", 0700);
        if (stat("storage/internal", &st) == -1) {
            mkdir("storage/internal", 0700);
            if (stat("storage/internal/games", &st) == -1) {
                mkdir("storage/internal/games", 0700);
                if (stat("storage/internal/games/com.mojang", &st) == -1) {
                    mkdir("storage/internal/games/com.mojang", 0700);
                    if (stat("storage/internal/games/com.mojang/minecraftpe", &st) == -1) {
                        mkdir("storage/internal/games/com.mojang/minecraftpe", 0700);
                    }
                }
            }
        }
        if (stat("storage/external", &st) == -1) {
            mkdir("storage/external", 0700);
        }
    }
    if (stat("mods", &st) == -1) {
        mkdir("mods", 0700);
    }
	readConfigFile();
    ninecraft_read_options_file(&platform_options, "options.txt");
    ninecraft_set_default_options(&platform_options, "options.txt");

    if (!glfwInit()) {
        // Initialization failed
        puts("init failed");
        return 1;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#ifdef _WIN32
    glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_NATIVE_CONTEXT_API);
#else
    glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_EGL_CONTEXT_API);
#endif
    _window = glfwCreateWindow(720, 480, "Ninecraft", NULL, NULL);
    if (!_window) {
        puts("Cant create window");
        return 1;
    }
    GLFWimage icon;
    icon.pixels = stbi_load("./res/drawable/iconx.png", &icon.width, &icon.height, NULL, STBI_rgb_alpha);
    glfwSetWindowIcon(_window, 1, &icon);
    stbi_image_free(icon.pixels);

    glfwSetKeyCallback(_window, key_callback);
    glfwSetCharCallback(_window, char_callback);
    glfwSetMouseButtonCallback(_window, mouse_click_callback);
    glfwSetScrollCallback(_window, mouse_scroll_callback);
    glfwSetCursorPosCallback(_window, mouse_pos_callback);
    glfwSetWindowSizeCallback(_window, resize_callback);
    glfwSetWindowCloseCallback(_window, window_close_callback);

    glfwMakeContextCurrent(_window);

    gladLoadGL(glfwGetProcAddress);

    audio_engine_init();

    gles_hook();
    missing_hook();
    add_custom_hook("__android_log_print", (void *)__android_log_print);
    stub_symbols(android_symbols, (void *)android_stub);
    stub_symbols(egl_symbols, (void *)egl_stub);
    add_custom_hook("SL_IID_VOLUME", &sles_iid_volume);
    add_custom_hook("SL_IID_ENGINE", &sles_iid_engine);
    add_custom_hook("SL_IID_BUFFERQUEUE", &sles_iid_bufferqueue);
    add_custom_hook("SL_IID_PLAY", &sles_iid_play);
    add_custom_hook("slCreateEngine", sles_create_engine);

#ifndef _WIN32
	add_custom_hook("__cxa_atexit", __cxa_atexit);
#endif

    handle = load_library("libminecraftpe.so");
    if (!handle) {
        puts("libminecraftpe.so not loaded");
        return 1;
    }
    
    android_alloc_setup_hooks(handle);
    android_string_setup_hooks(handle);

    static android_string_t in;
    android_string_cstr(&in, "v%d.%d.%d alpha");
#ifdef _MSC_VER
    void (__stdcall *get_game_version_string)(android_string_t *, android_string_t *) = (void (__stdcall *)(android_string_t *, android_string_t *))internal_dlsym(handle, "_ZN6Common20getGameVersionStringERKSs");

    void (__stdcall *get_game_version_string_2)(android_string_t *) = (void (__stdcall *)(android_string_t *))internal_dlsym(handle, "_ZN6Common20getGameVersionStringEv");
#else
    void (*get_game_version_string)(android_string_t *, android_string_t *) = (void (*)(android_string_t *, android_string_t *))internal_dlsym(handle, "_ZN6Common20getGameVersionStringERKSs");

    void (*get_game_version_string_2)(android_string_t *) = (void (*)(android_string_t *))internal_dlsym(handle, "_ZN6Common20getGameVersionStringEv");
#endif
    if (get_game_version_string != NULL) {
        static android_string_t game_version;
        get_game_version_string(&game_version, &in);
        char *verstr = android_string_to_str(&game_version);

        bool is_j = internal_dlsym(handle, "Java_com_mojang_minecraftpe_MainActivity_nativeOnCreate") != NULL;

        printf("Ninecraft is running mcpe %.6s%s\n", verstr, is_j ? "j" : "");

        if (strncmp(verstr, "v0.2.1", 6) == 0) {
            if (is_j) {
                version_id = version_id_0_2_1_j;
            } else {
                version_id = version_id_0_2_1;
            }
        } else if (strncmp(verstr, "v0.2.2", 6) == 0) {
            version_id = version_id_0_2_2;
        } else if (strncmp(verstr, "v0.3.0", 6) == 0) {
            if (is_j) {
                version_id = version_id_0_3_0_j;
            } else {
                version_id = version_id_0_3_0;
            }
        } else if (strncmp(verstr, "v0.3.2", 6) == 0) {
            if (is_j) {
                version_id = version_id_0_3_2_j;
            } else {
                version_id = version_id_0_3_2;
            }
        } else if (strncmp(verstr, "v0.3.3", 6) == 0) {
            if (is_j) {
                version_id = version_id_0_3_3_j;
            } else {
                version_id = version_id_0_3_3;
            }
        } else if (strncmp(verstr, "v0.4.0", 6) == 0) {
            if (is_j) {
                version_id = version_id_0_4_0_j;
            } else {
                version_id = version_id_0_4_0;
            }
        } else if (strncmp(verstr, "v0.5.0", 6) == 0) {
            if (is_j) {
                version_id = version_id_0_5_0_j;
            } else {
                version_id = version_id_0_5_0;
            }
        } else if (strncmp(verstr, "v0.6.0", 6) == 0) {
            version_id = version_id_0_6_0;
        } else if (strncmp(verstr, "v0.6.1", 6) == 0) {
            version_id = version_id_0_6_1;
        } else if (strncmp(verstr, "v0.7.0", 6) == 0) {
            version_id = version_id_0_7_0;
        } else if (strncmp(verstr, "v0.7.1", 6) == 0) {
            version_id = version_id_0_7_1;
        } else if (strncmp(verstr, "v0.7.2", 6) == 0) {
            version_id = version_id_0_7_2;
        } else if (strncmp(verstr, "v0.7.3", 6) == 0) {
            version_id = version_id_0_7_3;
        } else if (strncmp(verstr, "v0.7.4", 6) == 0) {
            version_id = version_id_0_7_4;
        } else if (strncmp(verstr, "v0.7.5", 6) == 0) {
            version_id = version_id_0_7_5;
        } else if (strncmp(verstr, "v0.7.6", 6) == 0) {
            version_id = version_id_0_7_6;
        } else {
            puts("Unsupported Version!");
            return 1;
        }
    } else if (get_game_version_string_2 != NULL) {
        static android_string_t game_version;
        get_game_version_string_2(&game_version);
        char *verstr = android_string_to_str(&game_version);
        printf("Ninecraft is running mcpe %s\n", verstr);
        if (strcmp(verstr, "v0.8.0 alpha") == 0) {
            version_id = version_id_0_8_0;
        } else if (strcmp(verstr, "v0.8.1 alpha") == 0) {
            version_id = version_id_0_8_1;
        } else if (strcmp(verstr, "v0.9.0 alpha") == 0) {
            version_id = version_id_0_9_0;
        } else if (strcmp(verstr, "v0.9.1 alpha") == 0) {
            version_id = version_id_0_9_1;
        } else if (strcmp(verstr, "v0.9.2 alpha") == 0) {
            version_id = version_id_0_9_2;
        } else if (strcmp(verstr, "v0.9.3 alpha") == 0) {
            version_id = version_id_0_9_3;
        } else if (strcmp(verstr, "v0.9.4 alpha") == 0) {
            version_id = version_id_0_9_4;
        } else if (strcmp(verstr, "v0.9.5 alpha") == 0) {
            version_id = version_id_0_9_5;
        } else if (strcmp(verstr, "v0.10.0 alpha") == 0) {
            version_id = version_id_0_10_0;
        } else if (strcmp(verstr, "v0.10.1 alpha") == 0) {
            version_id = version_id_0_10_1;
        } else if (strcmp(verstr, "v0.10.2 alpha") == 0) {
            version_id = version_id_0_10_2;
        } else if (strcmp(verstr, "v0.10.3 alpha") == 0) {
            version_id = version_id_0_10_3;
        } else if (strcmp(verstr, "v0.10.4 alpha") == 0) {
            version_id = version_id_0_10_4;
        } else if (strcmp(verstr, "v0.10.5 alpha") == 0) {
            version_id = version_id_0_10_5;
        } else {
            puts("Unsupported Version!");
            return 1;
        }
    } else {
        android_Dl_info info;
        android_dladdr(internal_dlsym(handle, "_ZN12NinecraftAppC2Ev"), &info);
        if (strncmp((char *)info.dli_fbase + 0x15d3a8, "v0.2.0", 6) == 0) { // v0.2.0
            version_id = version_id_0_2_0;
        } else if (strncmp((char *)info.dli_fbase + 0x15a480, "v0.2.0", 6) == 0) { // v0.2.0-demo
            version_id = version_id_0_2_0;
        } else if (strncmp((char *)info.dli_fbase + 0x1574f8, "v0.2.0", 6) == 0) { // v0.2.0j
            version_id = version_id_0_2_0_j;
        } else if (strncmp((char *)info.dli_fbase + 0x1545d8, "v0.2.0", 6) == 0) { // v0.2.0j-demo
            version_id = version_id_0_2_0_j;
        } else if (strncmp((char *)info.dli_fbase + 0xfca6c, "v0.1.3", 6) == 0) { // v0.1.3-2
            version_id = version_id_0_1_3;
        } else if (strncmp((char *)info.dli_fbase + 0xfca2c, "v0.1.3", 6) == 0) { // v0.1.3-1
            version_id = version_id_0_1_3;
        } else if (strncmp((char *)info.dli_fbase + 0xfae24, "v0.1.3", 6) == 0) { // v0.1.3-2-demo
            version_id = version_id_0_1_3;
        } else if (strncmp((char *)info.dli_fbase + 0xfade4, "v0.1.3", 6) == 0) { // v0.1.3-1-demo
            version_id = version_id_0_1_3;
        } else if (strncmp((char *)info.dli_fbase + 0x19d910, "v0.1.3", 6) == 0) { // v0.1.3j
            version_id = version_id_0_1_3_j;
        } else if (strncmp((char *)info.dli_fbase + 0x19a320, "v0.1.3", 6) == 0) { // v0.1.3j-demo
            version_id = version_id_0_1_3_j;
        } else if (strncmp((char *)info.dli_fbase + 0xf799c, "v0.1.2", 6) == 0) { // v0.1.2
            version_id = version_id_0_1_2;
        } else if (strncmp((char *)info.dli_fbase + 0xf5dc8, "v0.1.2", 6) == 0) { // v0.1.2-demo
            version_id = version_id_0_1_2;
        } else if (strncmp((char *)info.dli_fbase + 0x1966c0, "v0.1.2", 6) == 0) { // v0.1.2j
            version_id = version_id_0_1_2_j;
        } else if (strncmp((char *)info.dli_fbase + 0x193128, "v0.1.2", 6) == 0) { // v0.1.2j-demo
            version_id = version_id_0_1_2_j;
        } else if (strncmp((char *)info.dli_fbase + 0xf6d30, "v0.1.1", 6) == 0) { // v0.1.1
            version_id = version_id_0_1_1;
        } else if (strncmp((char *)info.dli_fbase + 0xf5154, "v0.1.1", 6) == 0) { // v0.1.1-demo
            version_id = version_id_0_1_1;
        } else if (strncmp((char *)info.dli_fbase + 0x19557c, "v0.1.1", 6) == 0) { // v0.1.1j
            version_id = version_id_0_1_1_j;
        } else if (strncmp((char *)info.dli_fbase + 0x191ce4, "v0.1.1", 6) == 0) { // v0.1.1j-demo
            version_id = version_id_0_1_1_j;
        } else if (strncmp((char *)info.dli_fbase + 0x119c64, "v0.1.0", 6) == 0) { // v0.1.0-touch
            version_id = version_id_0_1_0_touch;
        } else if (strncmp((char *)info.dli_fbase + 0x131480, "v0.1.0", 6) == 0) { // v0.1.0
            version_id = version_id_0_1_0;
        } else if (strncmp((char *)info.dli_fbase + 0x12f938, "v0.1.0", 6) == 0) { // v0.1.0-demo-canada
            version_id = version_id_0_1_0;
        } else if (strncmp((char *)info.dli_fbase + 0x12f968, "v0.1.0", 6) == 0) { // v0.1.0-demo
            version_id = version_id_0_1_0;
        } else {
            puts("Unsupported Version!");
            return 1;
        }
    }
    	
	if(version_id == version_id_0_9_5){ //0.9.5: fix space not working in chat(and remove its function from other screens
#ifdef __arm__
		printf("0.9.5 arm: cant disable key_jump in screen\n");
#else
		unsigned char* method = internal_dlsym(handle, "_ZN6Screen10keyPressedEi");
		method[155] = 0x90;
		method[156] = 0x90;
#endif
	}
	
	if(version_id == version_id_0_7_6){
		//0.7.6 x86: inv fix (it will not work in arm)
#ifdef __arm__
		printf("0.7.6 and arm: cant fix inv\n");
#else
		unsigned char* method = internal_dlsym(handle, "_ZN13ScreenChooser12createScreenE8ScreenId");
		method[42] = 0x90; //jz      short loc_1193B8 -> nop nop -> use touch:: inv
		method[43] = 0x90;
#endif

	}
    	
	if(opt_NO_KEYBOARD_SPACE_IN_CHAT.value.asbool){
#ifdef __arm
		printf("cant patch arm .so to include no keyboard space in chat patch!\n");
#else
		if(version_id == version_id_0_7_6){
			unsigned char* method = internal_dlsym(handle, "_ZN10ChatScreen6renderEiif");
			method[35] = 0xe9;
			method[36] = 0x70;
			method[37] = 0x01;
			method[38] = 0x00;
			method[39] = 0x00;

			method = internal_dlsym(handle, "_ZN10ChatScreen26updateToggleKeyboardButtonEv");
			method[248] = 0x90;
			method[249] = 0x90;
			
			method = internal_dlsym(handle, "_ZN10ChatScreen24updateKeyboardVisibilityEv");
			method[119] = 0x00;
			method[128] = 0x00;

		}else if(version_id == version_id_0_8_1){
			unsigned char* method = internal_dlsym(handle, "_ZN10ChatScreen6renderEiif");
			method[43] = 0xe9;
			method[44] = 0x68;
			method[45] = 0x01;
			method[46] = 0x00;
			method[47] = 0x00;

			method = internal_dlsym(handle, "_ZN10ChatScreen26updateToggleKeyboardButtonEv");
			method[248] = 0x90;
			method[249] = 0x90;
			
			method = internal_dlsym(handle, "_ZN10ChatScreen24updateKeyboardVisibilityEv");
			method[150] = 0x00;
			method[158] = 0x00;
		}else{
			printf("no keyboard space in chat patch is not implemented for %d\n", version_id);
		}
#endif
	}

	if(version_id == version_id_0_9_5){
#ifdef __arm__
		printf("cant inject!\n");
#else
		unsigned char* method = internal_dlsym(handle, "_ZN9Minecraft14useTouchscreenEv");
		DETOUR(method, Minecraft_useTouchscreen_095, true);
		method = internal_dlsym(handle, "_ZN3Gui11getNumSlotsEv");
		DETOUR(method, Gui_getNumSlots_095, true);
		//printf("usecontroller: %x %x %x %x %x %x\n", *method, method[1], method[2], method[3], method[4], method[5]);
#endif
	}
	if(version_id == version_id_0_10_5){
#ifdef __arm__
		printf("cant inject!\n");
#else
		MinecraftClient__reloadInput_0105_real = internal_dlsym(handle, "_ZN15MinecraftClient12_reloadInputEv");
		unsigned char* method = internal_dlsym(handle, "_ZN15MinecraftClient14useTouchscreenEv");
		DETOUR(method, MinecraftClient_useTouchscreen_0105, true);
		method = internal_dlsym(handle, "_ZN3Gui11getNumSlotsEv");
		DETOUR(method, Gui_getNumSlots_0105, true);
		//method = internal_dlsym(handle, "_ZN15MinecraftClient13useControllerEv");
		//DETOUR(method, MinecraftClient_useController_0105, true);
#endif
	}



    multitouch_setup_hooks(handle);
    keyboard_setup_hooks(handle);
    minecraft_setup_hooks(handle);
    inject_mods(handle, version_id);
    mod_loader_load_all();

    controller_states = (unsigned char *)internal_dlsym(handle, "_ZN10Controller15isTouchedValuesE");
    controller_x_stick = (float *)internal_dlsym(handle, "_ZN10Controller12stickValuesXE");
    controller_y_stick = (float *)internal_dlsym(handle, "_ZN10Controller12stickValuesYE");

    if (version_id > version_id_0_7_6 && version_id <= version_id_0_9_5) {
        default_mouse_mode = GLFW_CURSOR_HIDDEN;
    }

    glfwSetInputMode(_window, GLFW_CURSOR, default_mouse_mode);

    printf("%s\n", glGetString(GL_VERSION));

    if (version_id >= version_id_0_9_0 && version_id <= version_id_0_9_5) {
        printf("nine construct %p\n", ninecraft_app_construct_2);
    } else if (version_id >= version_id_0_10_0) {
        printf("nine construct %p\n", minecraft_client_construct);
    } else {
        printf("nine construct %p\n", ninecraft_app_construct);
    }

    size_t ninecraft_app_size;

    if (version_id == version_id_0_1_0) {
        ninecraft_app_size = NINECRAFTAPP_SIZE_0_1_0;
    } else if (version_id == version_id_0_1_0_touch) {
        ninecraft_app_size = NINECRAFTAPP_SIZE_0_1_0_TOUCH;
    } else if (version_id == version_id_0_1_1) {
        ninecraft_app_size = NINECRAFTAPP_SIZE_0_1_1;
    } else if (version_id == version_id_0_1_1_j) {
        ninecraft_app_size = NINECRAFTAPP_SIZE_0_1_1_J;
    } else if (version_id == version_id_0_1_2) {
        ninecraft_app_size = NINECRAFTAPP_SIZE_0_1_2;
    } else if (version_id == version_id_0_1_2_j) {
        ninecraft_app_size = NINECRAFTAPP_SIZE_0_1_2_J;
    } else if (version_id == version_id_0_1_3) {
        ninecraft_app_size = NINECRAFTAPP_SIZE_0_1_3;
    } else if (version_id == version_id_0_1_3_j) {
        ninecraft_app_size = NINECRAFTAPP_SIZE_0_1_3_J;
    } else if (version_id == version_id_0_2_0) {
        ninecraft_app_size = NINECRAFTAPP_SIZE_0_2_0;
    } else if (version_id == version_id_0_2_0_j) {
        ninecraft_app_size = NINECRAFTAPP_SIZE_0_2_0_J;
    } else if (version_id == version_id_0_2_1) {
        ninecraft_app_size = NINECRAFTAPP_SIZE_0_2_1;
    } else if (version_id == version_id_0_2_1_j) {
        ninecraft_app_size = NINECRAFTAPP_SIZE_0_2_1_J;
    } else if (version_id == version_id_0_2_2) {
        ninecraft_app_size = NINECRAFTAPP_SIZE_0_2_2;
    } else if (version_id == version_id_0_3_0) {
        ninecraft_app_size = NINECRAFTAPP_SIZE_0_3_0;
    } else if (version_id == version_id_0_3_0_j) {
        ninecraft_app_size = NINECRAFTAPP_SIZE_0_3_0_J;
    } else if (version_id == version_id_0_3_2) {
        ninecraft_app_size = NINECRAFTAPP_SIZE_0_3_2;
    } else if (version_id == version_id_0_3_2_j) {
        ninecraft_app_size = NINECRAFTAPP_SIZE_0_3_2_J;
    } else if (version_id == version_id_0_3_3) {
        ninecraft_app_size = NINECRAFTAPP_SIZE_0_3_3;
    } else if (version_id == version_id_0_3_3_j) {
        ninecraft_app_size = NINECRAFTAPP_SIZE_0_3_3_J;
    } else if (version_id == version_id_0_4_0) {
        ninecraft_app_size = NINECRAFTAPP_SIZE_0_4_0;
    } else if (version_id == version_id_0_5_0) {
        ninecraft_app_size = NINECRAFTAPP_SIZE_0_5_0;
    } else if (version_id == version_id_0_5_0_j) {
        ninecraft_app_size = NINECRAFTAPP_SIZE_0_5_0_J;
    } else if (version_id == version_id_0_6_0) {
        ninecraft_app_size = NINECRAFTAPP_SIZE_0_6_0;
    } else if (version_id == version_id_0_6_1) {
        ninecraft_app_size = NINECRAFTAPP_SIZE_0_6_1;
    } else if (version_id == version_id_0_7_0) {
        ninecraft_app_size = NINECRAFTAPP_SIZE_0_7_0;
    } else if (version_id == version_id_0_7_1) {
        ninecraft_app_size = NINECRAFTAPP_SIZE_0_7_1;
    } else if (version_id == version_id_0_7_2) {
        ninecraft_app_size = NINECRAFTAPP_SIZE_0_7_2;
    } else if (version_id == version_id_0_7_3) {
        ninecraft_app_size = NINECRAFTAPP_SIZE_0_7_3;
    } else if (version_id == version_id_0_7_4) {
        ninecraft_app_size = NINECRAFTAPP_SIZE_0_7_4;
    } else if (version_id == version_id_0_7_5) {
        ninecraft_app_size = NINECRAFTAPP_SIZE_0_7_5;
    } else if (version_id == version_id_0_7_6) {
        ninecraft_app_size = NINECRAFTAPP_SIZE_0_7_6;
    } else if (version_id == version_id_0_8_0) {
        ninecraft_app_size = NINECRAFTAPP_SIZE_0_8_0;
    } else if (version_id == version_id_0_8_1) {
        ninecraft_app_size = NINECRAFTAPP_SIZE_0_8_1;
    } else if (version_id == version_id_0_9_0) {
        ninecraft_app_size = NINECRAFTAPP_SIZE_0_9_0;
    } else if (version_id == version_id_0_9_1) {
        ninecraft_app_size = NINECRAFTAPP_SIZE_0_9_1;
    } else if (version_id == version_id_0_9_2) {
        ninecraft_app_size = NINECRAFTAPP_SIZE_0_9_2;
    } else if (version_id == version_id_0_9_3) {
        ninecraft_app_size = NINECRAFTAPP_SIZE_0_9_3;
    } else if (version_id == version_id_0_9_4) {
        ninecraft_app_size = NINECRAFTAPP_SIZE_0_9_4;
    } else if (version_id == version_id_0_9_5) {
        ninecraft_app_size = NINECRAFTAPP_SIZE_0_9_5;
    } else if (version_id == version_id_0_10_0) {
        ninecraft_app_size = MINECRAFTCLIENT_SIZE_0_10_0;
    } else if (version_id == version_id_0_10_1) {
        ninecraft_app_size = MINECRAFTCLIENT_SIZE_0_10_1;
    } else if (version_id == version_id_0_10_2) {
        ninecraft_app_size = MINECRAFTCLIENT_SIZE_0_10_2;
    } else if (version_id == version_id_0_10_3) {
        ninecraft_app_size = MINECRAFTCLIENT_SIZE_0_10_3;
    } else if (version_id == version_id_0_10_4) {
        ninecraft_app_size = MINECRAFTCLIENT_SIZE_0_10_4;
    } else if (version_id == version_id_0_10_5) {
        ninecraft_app_size = MINECRAFTCLIENT_SIZE_0_10_5;
    }
    ninecraft_app = malloc(ninecraft_app_size);
    if (version_id >= version_id_0_9_0 && version_id <= version_id_0_9_5) {
        ninecraft_app_construct_2(ninecraft_app, 0, NULL);
    } else if (version_id >= version_id_0_10_0) {
        minecraft_client_construct(ninecraft_app, 0, NULL);
    } else {
        ninecraft_app_construct(ninecraft_app);
    }

    char *int_path = (char *)malloc(1024);
    char *ext_path = (char *)malloc(1024);
    int_path[0] = '\0';
    ext_path[0] = '\0';
    getcwd(int_path, 1024);
    getcwd(ext_path, 1024);
    strcat(int_path, "/storage/internal/");
    strcat(ext_path, "/storage/external/");

    uintptr_t int_off = get_ninecraftapp_internal_storage_offset(version_id);
    uintptr_t ext_off = get_ninecraftapp_external_storage_offset(version_id);
    if (int_off) {
        android_string_equ((android_string_t *)((char *)ninecraft_app + int_off), int_path);
    }
    if (ext_off) {
        android_string_equ((android_string_t *)((char *)ninecraft_app + ext_off), ext_path);
    }

    if (version_id >= version_id_0_9_0) {
        app_platform_0_9_0_t *plat = malloc(sizeof(app_platform_0_9_0_t));
        static app_context_0_9_0_t context = {
            .egl_context = NULL,
            .egl_display = NULL,
            .egl_surface = NULL,
            .u0 = NULL,
            .platform = NULL,
            .do_render = false
        };

        app_platform_construct(plat);
        if (version_id >= version_id_0_9_0 && version_id <= version_id_0_9_5) {
            memcpy(&platform_vtable_0_9_0, plat->vtable, sizeof(app_platform_vtable_0_9_0_t));
            plat->vtable = (void **)&platform_vtable_0_9_0;
            platform_vtable_0_9_0.getImagePath = (void *)GET_SYSV_WRAPPER(AppPlatform_linux$getImagePath);
            platform_vtable_0_9_0.loadPNG = (void *)AppPlatform_linux$loadPNG_0_9_0;
            platform_vtable_0_9_0.loadTGA = (void *)AppPlatform_linux$loadTGA_0_9_0;
            platform_vtable_0_9_0.getDateString = (void *)GET_SYSV_WRAPPER(AppPlatform_linux$getDateStringGNU);
            platform_vtable_0_9_0.readAssetFile = (void *)GET_SYSV_WRAPPER(AppPlatform_linux$readAssetFile_0_9_0);
            platform_vtable_0_9_0.getScreenHeight = (void *)AppPlatform_linux$getScreenHeight;
            platform_vtable_0_9_0.getScreenWidth = (void *)AppPlatform_linux$getScreenWidth;
            platform_vtable_0_9_0.showKeyboard = (void *)AppPlatform_linux$showKeyboard;
            platform_vtable_0_9_0.hideKeyboard = (void *)AppPlatform_linux$hideKeyboard;
            platform_vtable_0_9_0.getBroadcastAddresses = (void *)GET_SYSV_WRAPPER(AppPlatform_linux$getBroadcastAddresses);
            platform_vtable_0_9_0.getAvailableMemory = (void *)AppPlatform_linux$getAvailableMemory;
            platform_vtable_0_9_0.getPlatformStringVar = (void *)GET_SYSV_WRAPPER(AppPlatform_linux$getPlatformStringVarGNU);
            platform_vtable_0_9_0.getLoginInformation = (void *)GET_SYSV_WRAPPER(AppPlatform_linux$getLoginInformation);
            platform_vtable_0_9_0.setLoginInformation = (void *)AppPlatform_linux$setLoginInformation;
            platform_vtable_0_9_0.supportsTouchscreen = (void *)AppPlatform_linux$supportsTouchscreen;
            platform_vtable_0_9_0.isNetworkEnabled = (void *)AppPlatform_linux$isNetworkEnabled;
            platform_vtable_0_9_0.getPixelsPerMillimeter = (void *)AppPlatform_linux$getPixelsPerMillimeter;
        } else if (version_id >= version_id_0_10_0 && version_id <= version_id_0_10_5) {
            memcpy(&platform_vtable_0_10_0, plat->vtable, sizeof(app_platform_vtable_0_10_0_t));
            plat->vtable = (void **)&platform_vtable_0_10_0;
            platform_vtable_0_10_0.getImagePath = (void *)GET_SYSV_WRAPPER(AppPlatform_linux$getImagePath);
            platform_vtable_0_10_0.loadPNG = (void *)AppPlatform_linux$loadPNG_0_9_0;
            platform_vtable_0_10_0.loadTGA = (void *)AppPlatform_linux$loadTGA_0_9_0;
            platform_vtable_0_10_0.getDateString = (void *)GET_SYSV_WRAPPER(AppPlatform_linux$getDateStringGNU);
            platform_vtable_0_10_0.readAssetFile = (void *)GET_SYSV_WRAPPER(AppPlatform_linux$readAssetFile_0_9_0);
            platform_vtable_0_10_0.getScreenHeight = (void *)AppPlatform_linux$getScreenHeight;
            platform_vtable_0_10_0.getScreenWidth = (void *)AppPlatform_linux$getScreenWidth;
            platform_vtable_0_10_0.showKeyboard = (void *)AppPlatform_linux$showKeyboard;
            platform_vtable_0_10_0.hideKeyboard = (void *)AppPlatform_linux$hideKeyboard;
            platform_vtable_0_10_0.getBroadcastAddresses = (void *)GET_SYSV_WRAPPER(AppPlatform_linux$getBroadcastAddresses);
            platform_vtable_0_10_0.getAvailableMemory = (void *)AppPlatform_linux$getAvailableMemory;
            platform_vtable_0_10_0.getPlatformStringVar = (void *)GET_SYSV_WRAPPER(AppPlatform_linux$getPlatformStringVarGNU);
            platform_vtable_0_10_0.getLoginInformation = (void *)GET_SYSV_WRAPPER(AppPlatform_linux$getLoginInformation);
            platform_vtable_0_10_0.setLoginInformation = (void *)AppPlatform_linux$setLoginInformation;
            platform_vtable_0_10_0.supportsTouchscreen = (void *)AppPlatform_linux$supportsTouchscreen;
            platform_vtable_0_10_0.isNetworkEnabled = (void *)AppPlatform_linux$isNetworkEnabled;
            platform_vtable_0_10_0.getPixelsPerMillimeter = (void *)AppPlatform_linux$getPixelsPerMillimeter;
            platform_vtable_0_10_0.swapBuffers = (void *)AppPlatform_linux$swapBuffers;
        }
        context.platform = plat;
        app_init(ninecraft_app, &context);
    } else {
        AppPlatform_linux$AppPlatform_linux(&platform, handle, version_id);
        printf("%p\n", &platform);

        *(void **)((char *)ninecraft_app + 0x08) = NULL;      // egl_display
        *(void **)((char *)ninecraft_app + 0x0c) = NULL;      // egl_content
        *(void **)((char *)ninecraft_app + 0x10) = NULL;      // egl_surface
        *(void **)((char *)ninecraft_app + 0x14) = &platform; // app_platform

        if (version_id >= version_id_0_1_0_touch) {
            *(uint8_t *)((char *)ninecraft_app + 0x18) = 0; // do_render
        }

        ninecraft_app_init(ninecraft_app);

        if (version_id >= version_id_0_1_0_touch) {
            *(uint8_t *)((char *)ninecraft_app + 4) = 1; // is_inited
        }
    }

    if (version_id >= version_id_0_1_0_touch) {
        set_ninecraft_size(720, 480);
    } else {
        set_ninecraft_size_0_1_0(720, 480);
    }

    size_t minecraft_isgrabbed_offset;
    if (version_id == version_id_0_10_5) {
        minecraft_isgrabbed_offset = MINECRAFTCLIENT_ISGRABBED_OFFSET_0_10_5;
    } else if (version_id == version_id_0_10_4) {
        minecraft_isgrabbed_offset = MINECRAFTCLIENT_ISGRABBED_OFFSET_0_10_4;
    } else if (version_id == version_id_0_10_3) {
        minecraft_isgrabbed_offset = MINECRAFTCLIENT_ISGRABBED_OFFSET_0_10_3;
    } else if (version_id == version_id_0_10_2) {
        minecraft_isgrabbed_offset = MINECRAFTCLIENT_ISGRABBED_OFFSET_0_10_2;
    } else if (version_id == version_id_0_10_1) {
        minecraft_isgrabbed_offset = MINECRAFTCLIENT_ISGRABBED_OFFSET_0_10_1;
    } else if (version_id == version_id_0_10_0) {
        minecraft_isgrabbed_offset = MINECRAFTCLIENT_ISGRABBED_OFFSET_0_10_0;
    } else if (version_id == version_id_0_9_5) {
        minecraft_isgrabbed_offset = MINECRAFT_ISGRABBED_OFFSET_0_9_5;
    } else if (version_id == version_id_0_9_4) {
        minecraft_isgrabbed_offset = MINECRAFT_ISGRABBED_OFFSET_0_9_4;
    } else if (version_id == version_id_0_9_3) {
        minecraft_isgrabbed_offset = MINECRAFT_ISGRABBED_OFFSET_0_9_3;
    } else if (version_id == version_id_0_9_2) {
        minecraft_isgrabbed_offset = MINECRAFT_ISGRABBED_OFFSET_0_9_2;
    } else if (version_id == version_id_0_9_1) {
        minecraft_isgrabbed_offset = MINECRAFT_ISGRABBED_OFFSET_0_9_1;
    } else if (version_id == version_id_0_9_0) {
        minecraft_isgrabbed_offset = MINECRAFT_ISGRABBED_OFFSET_0_9_0;
    } else if (version_id == version_id_0_8_1) {
        minecraft_isgrabbed_offset = MINECRAFT_ISGRABBED_OFFSET_0_8_1;
    } else if (version_id == version_id_0_8_0) {
        minecraft_isgrabbed_offset = MINECRAFT_ISGRABBED_OFFSET_0_8_0;
    } else if (version_id == version_id_0_7_6) {
        minecraft_isgrabbed_offset = MINECRAFT_ISGRABBED_OFFSET_0_7_6;
    } else if (version_id == version_id_0_7_5) {
        minecraft_isgrabbed_offset = MINECRAFT_ISGRABBED_OFFSET_0_7_5;
    } else if (version_id == version_id_0_7_4) {
        minecraft_isgrabbed_offset = MINECRAFT_ISGRABBED_OFFSET_0_7_4;
    } else if (version_id == version_id_0_7_3) {
        minecraft_isgrabbed_offset = MINECRAFT_ISGRABBED_OFFSET_0_7_3;
    } else if (version_id == version_id_0_7_2) {
        minecraft_isgrabbed_offset = MINECRAFT_ISGRABBED_OFFSET_0_7_2;
    } else if (version_id == version_id_0_7_1) {
        minecraft_isgrabbed_offset = MINECRAFT_ISGRABBED_OFFSET_0_7_1;
    } else if (version_id == version_id_0_7_0) {
        minecraft_isgrabbed_offset = MINECRAFT_ISGRABBED_OFFSET_0_7_0;
    } else if (version_id == version_id_0_6_1) {
        minecraft_isgrabbed_offset = MINECRAFT_ISGRABBED_OFFSET_0_6_1;
    } else if (version_id == version_id_0_6_0) {
        minecraft_isgrabbed_offset = MINECRAFT_ISGRABBED_OFFSET_0_6_0;
    } else if (version_id == version_id_0_5_0) {
        minecraft_isgrabbed_offset = MINECRAFT_ISGRABBED_OFFSET_0_5_0;
    } else if (version_id == version_id_0_5_0_j) {
        minecraft_isgrabbed_offset = MINECRAFT_ISGRABBED_OFFSET_0_5_0_J;
    } else if (version_id == version_id_0_4_0) {
        minecraft_isgrabbed_offset = MINECRAFT_ISGRABBED_OFFSET_0_4_0;
    } else if (version_id == version_id_0_4_0_j) {
        minecraft_isgrabbed_offset = MINECRAFT_ISGRABBED_OFFSET_0_4_0_J;
    } else if (version_id == version_id_0_3_3) {
        minecraft_isgrabbed_offset = MINECRAFT_ISGRABBED_OFFSET_0_3_3;
    } else if (version_id == version_id_0_3_3_j) {
        minecraft_isgrabbed_offset = MINECRAFT_ISGRABBED_OFFSET_0_3_3_J;
    } else if (version_id == version_id_0_3_2) {
        minecraft_isgrabbed_offset = MINECRAFT_ISGRABBED_OFFSET_0_3_2;
    } else if (version_id == version_id_0_3_2_j) {
        minecraft_isgrabbed_offset = MINECRAFT_ISGRABBED_OFFSET_0_3_2_J;
    } else if (version_id == version_id_0_3_0) {
        minecraft_isgrabbed_offset = MINECRAFT_ISGRABBED_OFFSET_0_3_0;
    } else if (version_id == version_id_0_3_0_j) {
        minecraft_isgrabbed_offset = MINECRAFT_ISGRABBED_OFFSET_0_3_0_J;
    } else if (version_id == version_id_0_2_2) {
        minecraft_isgrabbed_offset = MINECRAFT_ISGRABBED_OFFSET_0_2_2;
    } else if (version_id == version_id_0_2_1) {
        minecraft_isgrabbed_offset = MINECRAFT_ISGRABBED_OFFSET_0_2_1;
    } else if (version_id == version_id_0_2_1_j) {
        minecraft_isgrabbed_offset = MINECRAFT_ISGRABBED_OFFSET_0_2_1_J;
    } else if (version_id == version_id_0_2_0) {
        minecraft_isgrabbed_offset = MINECRAFT_ISGRABBED_OFFSET_0_2_0;
    } else if (version_id == version_id_0_2_0_j) {
        minecraft_isgrabbed_offset = MINECRAFT_ISGRABBED_OFFSET_0_2_0_J;
    } else if (version_id == version_id_0_1_3) {
        minecraft_isgrabbed_offset = MINECRAFT_ISGRABBED_OFFSET_0_1_3;
    } else if (version_id == version_id_0_1_3_j) {
        minecraft_isgrabbed_offset = MINECRAFT_ISGRABBED_OFFSET_0_1_3_J;
    } else if (version_id == version_id_0_1_2) {
        minecraft_isgrabbed_offset = MINECRAFT_ISGRABBED_OFFSET_0_1_2;
    } else if (version_id == version_id_0_1_2_j) {
        minecraft_isgrabbed_offset = MINECRAFT_ISGRABBED_OFFSET_0_1_2_J;
    } else if (version_id == version_id_0_1_1) {
        minecraft_isgrabbed_offset = MINECRAFT_ISGRABBED_OFFSET_0_1_1;
    } else if (version_id == version_id_0_1_1_j) {
        minecraft_isgrabbed_offset = MINECRAFT_ISGRABBED_OFFSET_0_1_1_J;
    } else if (version_id == version_id_0_1_0_touch) {
        minecraft_isgrabbed_offset = MINECRAFT_ISGRABBED_OFFSET_0_1_0_TOUCH;
    } else if (version_id == version_id_0_1_0) {
        minecraft_isgrabbed_offset = MINECRAFT_ISGRABBED_OFFSET_0_1_0;
    }

	if(version_id == version_id_0_9_5){
		int** playervt = (int**)((int)internal_dlsym(handle, "_ZTV11LocalPlayer") + 8);
		printf("'issneaking %p\n", playervt[44]);
		playervt[44] = (int*) &LocalPlayer_isSneaking_hook_095;
	}
	
	if (version_id == version_id_0_8_1) {
		int** vtinput = (int**)((int)internal_dlsym(handle, "_ZTV15XperiaPlayInput") + 8);
		XperialPlayInput_tick_real_081 = (void*) vtinput[2];
		vtinput[2] = (int*) &XperialPlayInput_tick_hook_081;
	}
	if (version_id == version_id_0_9_5) {
		int** vtinput = (int**)((int)internal_dlsym(handle, "_ZTV19ControllerMoveInput") + 8);
		ControllerMoveInput_tick_real_095 = (void*) vtinput[2];
		vtinput[2] = (int*) &ControllerMoveInput_tick_hook_095;
	}

    while (true) {
        if (((bool *)ninecraft_app)[minecraft_isgrabbed_offset]) {
            if (!mouse_pointer_hidden) {
                grab_mouse();
            }
        } else {
            if (mouse_pointer_hidden) {
                release_mouse();
            }
        }
        if (mouse_pointer_hidden) {
            controller_states[1] = 1;
            controller_y_stick[1] = (float)(y_cam - 180.0) * 0.0055555557;
            controller_x_stick[1] = ((float)((x_cam - 483.0)) * 0.0020703934);
        }

        if(additionalBuildChecks){
        	if(version_id == version_id_0_8_1){
        		int placeActionTimeout = *(int*)((int)ninecraft_app + 3332);

        		if(placeActionTimeout){
        			int tickDiff = *(unsigned int*)(ninecraft_app + 3336) - lastBuiltTick;

        			if(tickDiff > 8){
        				*(int*)((int)ninecraft_app + 3332) = 0;
        				int state = glfwGetMouseButton(_window, GLFW_MOUSE_BUTTON_RIGHT);
        				if (state == GLFW_PRESS)
        				{
        					mouse_click_callback(_window, GLFW_MOUSE_BUTTON_RIGHT, state, 0);
        				}

        			}

        		}

        	}
        }

        if (version_id >= version_id_0_10_0) {
            minecraft_update(ninecraft_app);
        } else {
            ninecraft_app_update(ninecraft_app);
        }

        if (!mouse_pointer_hidden) {
            if (version_id > version_id_0_7_6 && version_id <= version_id_0_9_5) {
                float inv_gui_scale = *((float *)internal_dlsym(handle, "_ZN3Gui11InvGuiScaleE"));
                double xpos, ypos;
                glfwGetCursorPos(_window, &xpos, &ypos);
                short cx = (short)(xpos * inv_gui_scale);
                short cy = (short)(ypos * inv_gui_scale);
                ((NINECRAFT_FLOAT_FUNC void (*)(float, float, void *))internal_dlsym(handle, "_Z12renderCursorffP9Minecraft"))(cx, cy, ninecraft_app);
            }
        }
        audio_engine_tick();
        glfwSwapBuffers(_window);
        glfwPollEvents();
    }
    return 0;
}
