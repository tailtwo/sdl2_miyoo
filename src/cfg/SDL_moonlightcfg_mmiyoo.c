#include <string.h>
#include <stdio.h> 
#include <unistd.h>
#include <json-c/json.h>

#include <SDL2/SDL.h>
#include "../../cfg/SDL_moonlightcfg_mmiyoo.h"

MOON moon = {0};
int cpuclock = 0;

int cfgReadMouse(struct json_object *jfile) {
    struct json_object *jval = NULL;
    struct json_object *jScaleFactor = NULL;
    struct json_object *jAcceleration = NULL;
    struct json_object *jAccelerationRate = NULL;
    struct json_object *jMaxAcceleration = NULL;
    json_object_object_get_ex(jfile, "mouse", &jval);

    if (jval) {
        json_object_object_get_ex(jval, "scaleFactor", &jScaleFactor);
        if (jScaleFactor) {
            moon.mouse.scaleFactor = json_object_get_int(jScaleFactor);
        } else {
            moon.mouse.scaleFactor = MMIYOO_DEFAULT_SCALE_FACTOR;
        }
        
        json_object_object_get_ex(jval, "acceleration", &jAcceleration);
        if (jAcceleration) {
            moon.mouse.acceleration = json_object_get_double(jAcceleration);
        } else {
            moon.mouse.acceleration = MMIYOO_DEFAULT_ACCELERATION;
        }

        json_object_object_get_ex(jval, "accelerationRate", &jAccelerationRate);
        if (jAccelerationRate) {
            moon.mouse.accelerationRate = json_object_get_double(jAccelerationRate);
        } else {
            moon.mouse.accelerationRate = MMIYOO_DEFAULT_ACCELERATION_RATE;
        }

        json_object_object_get_ex(jval, "maxAcceleration", &jMaxAcceleration);
        if (jMaxAcceleration) {
            moon.mouse.maxAcceleration = json_object_get_double(jMaxAcceleration);
        } else {
            moon.mouse.maxAcceleration = MMIYOO_DEFAULT_MAX_ACCELERATION;
        }

        printf("[json] Mouse settings: scaleFactor=%d, acceleration=%f, accelerationRate=%f, maxAcceleration=%f\n",
               moon.mouse.scaleFactor, moon.mouse.acceleration, moon.mouse.accelerationRate, moon.mouse.maxAcceleration);
    } else {
        printf("Mouse settings not found in json file. Using defaults.\n");
        moon.mouse.scaleFactor = MMIYOO_DEFAULT_SCALE_FACTOR;
        moon.mouse.acceleration = MMIYOO_DEFAULT_ACCELERATION;
        moon.mouse.accelerationRate = MMIYOO_DEFAULT_ACCELERATION_RATE;
        moon.mouse.maxAcceleration = MMIYOO_DEFAULT_MAX_ACCELERATION;
    }
    return 0;
}

int cfgReadClock(struct json_object *jfile) {
    struct json_object *jval = NULL;
    json_object_object_get_ex(jfile, "cpuclock", &jval);

    if (jval) {
        const char *cpuclock_str = json_object_get_string(jval);
        if (cpuclock_str) {
            int cpuclock = atoi(cpuclock_str);
            if (cpuclock > 1950) {
                printf("[json] moon.cpuclock too high! Using default: %d\n", MMIYOO_DEFAULT_CPU_CLOCK);
                moon.cpuclock = MMIYOO_DEFAULT_CPU_CLOCK;
            } else if (cpuclock < 1200) {
                printf("[json] moon.cpuclock too low! Using default: %d\n", MMIYOO_DEFAULT_CPU_CLOCK);
                moon.cpuclock = MMIYOO_DEFAULT_CPU_CLOCK;
            } else if (cpuclock >= 1200 && cpuclock <= 1950) {
                printf("[json] moon.cpuclock: %d\n", cpuclock);
                moon.cpuclock = cpuclock;
            } else {
                printf("Invalid moon.cpuclock value. Using default: %d\n", MMIYOO_DEFAULT_CPU_CLOCK);
                moon.cpuclock = MMIYOO_DEFAULT_CPU_CLOCK;
            }
        } else {
            printf("Invalid moon.cpuclock value. Using default: %d\n", MMIYOO_DEFAULT_CPU_CLOCK);
            moon.cpuclock = MMIYOO_DEFAULT_CPU_CLOCK;
        }
    } else {
        printf("moon.cpuclock not found in json file. Using default: %d.\n", MMIYOO_DEFAULT_CPU_CLOCK);
        moon.cpuclock = MMIYOO_DEFAULT_CPU_CLOCK;
    }
    return 0;
}

int cfgReadCustomKeys(struct json_object *jfile) {
    struct json_object *jCustomKeys = NULL;
    SDL_Keycode default_keycode = SDLK_UNKNOWN;
    SDL_Keycode received_keycode = SDLK_UNKNOWN;
    const char *keys[] = {"A", "B", "X", "Y", "L1", "L2", "R1", "R2", "LeftDpad", "RightDpad", "UpDpad", "DownDpad", "Start", "Select", "Menu"};
    int numKeys = sizeof(keys) / sizeof(char*);
    struct json_object *jval = NULL;
    
    json_object_object_get_ex(jfile, "customkeys", &jCustomKeys);
    if (!jCustomKeys) {
        printf("customkeys block not found in json file. Using defaults.\n");
        return 0;
    }

    for (int i = 0; i < numKeys; ++i) {
        json_object_object_get_ex(jCustomKeys, keys[i], &jval);

        
        if (strcmp(keys[i], "A") == 0) default_keycode = MMIYOO_DEFAULT_KEY_A;
        else if (strcmp(keys[i], "B") == 0) default_keycode = MMIYOO_DEFAULT_KEY_B;
        else if (strcmp(keys[i], "X") == 0) default_keycode = MMIYOO_DEFAULT_KEY_X;
        else if (strcmp(keys[i], "Y") == 0) default_keycode = MMIYOO_DEFAULT_KEY_Y;
        else if (strcmp(keys[i], "L1") == 0) default_keycode = MMIYOO_DEFAULT_KEY_L1;
        else if (strcmp(keys[i], "L2") == 0) default_keycode = MMIYOO_DEFAULT_KEY_L2;
        else if (strcmp(keys[i], "R1") == 0) default_keycode = MMIYOO_DEFAULT_KEY_R1;
        else if (strcmp(keys[i], "R2") == 0) default_keycode = MMIYOO_DEFAULT_KEY_R2;
        else if (strcmp(keys[i], "LeftDpad") == 0) default_keycode = MMIYOO_DEFAULT_KEY_LeftDpad;
        else if (strcmp(keys[i], "RightDpad") == 0) default_keycode = MMIYOO_DEFAULT_KEY_RightDpad;
        else if (strcmp(keys[i], "UpDpad") == 0) default_keycode = MMIYOO_DEFAULT_KEY_UpDpad;
        else if (strcmp(keys[i], "DownDpad") == 0) default_keycode = MMIYOO_DEFAULT_KEY_DownDpad;
        else if (strcmp(keys[i], "Start") == 0) default_keycode = MMIYOO_DEFAULT_KEY_Start;
        else if (strcmp(keys[i], "Select") == 0) default_keycode = MMIYOO_DEFAULT_KEY_Select;
        else if (strcmp(keys[i], "Menu") == 0) default_keycode = MMIYOO_DEFAULT_KEY_MENU;
        
        if (jval) {
            received_keycode = SDL_GetKeyFromName(json_object_get_string(jval));
        }
        
        if (strcmp(keys[i], "A") == 0) {
            moon.customkey.A = (jval) ? received_keycode : default_keycode;
        } else if (strcmp(keys[i], "B") == 0) {
            moon.customkey.B = (jval) ? received_keycode : default_keycode;
        } else if (strcmp(keys[i], "X") == 0) {
            moon.customkey.X = (jval) ? received_keycode : default_keycode;
        } else if (strcmp(keys[i], "Y") == 0) {
            moon.customkey.Y = (jval) ? received_keycode : default_keycode;
        } else if (strcmp(keys[i], "L1") == 0) {
            moon.customkey.L1 = (jval) ? received_keycode : default_keycode;
        } else if (strcmp(keys[i], "L2") == 0) {
            moon.customkey.L2 = (jval) ? received_keycode : default_keycode;
        } else if (strcmp(keys[i], "R1") == 0) {
            moon.customkey.R1 = (jval) ? received_keycode : default_keycode;
        } else if (strcmp(keys[i], "R2") == 0) {
            moon.customkey.R2 = (jval) ? received_keycode : default_keycode;
        } else if (strcmp(keys[i], "LeftDpad") == 0) {
            moon.customkey.LeftDpad = (jval) ? received_keycode : default_keycode;
        } else if (strcmp(keys[i], "RightDpad") == 0) {
            moon.customkey.RightDpad = (jval) ? received_keycode : default_keycode;
        } else if (strcmp(keys[i], "UpDpad") == 0) {
            moon.customkey.UpDpad = (jval) ? received_keycode : default_keycode;
        } else if (strcmp(keys[i], "DownDpad") == 0) {
            moon.customkey.DownDpad = (jval) ? received_keycode : default_keycode;
        } else if (strcmp(keys[i], "Start") == 0) {
            moon.customkey.Start = (jval) ? received_keycode : default_keycode;
        } else if (strcmp(keys[i], "Select") == 0) {
            moon.customkey.Select = (jval) ? received_keycode : default_keycode;
        } else if (strcmp(keys[i], "Menu") == 0) {
            moon.customkey.Menu = (jval) ? received_keycode : default_keycode;
        }
        
        if (jval) {
            if (received_keycode != SDLK_UNKNOWN) {
                printf("[json] moon.customkey.%s: %d\n", keys[i], received_keycode);
            } else {
                printf("Invalid moon.customkey.%s, reset as %d\n", keys[i], default_keycode);
            }
        }
    }
    return 0;
}

SDL_Keycode stringToKeycode(const char *keyString) {
    char upperKeyString[strlen(keyString) + 1];
    for (size_t i = 0; i < strlen(keyString); ++i) {
        upperKeyString[i] = toupper(keyString[i]);
    }
    upperKeyString[strlen(keyString)] = '\0';
    
    if (strlen(upperKeyString) == 1 && isalpha(upperKeyString[0])) {
        return (SDL_Keycode)tolower(upperKeyString[0]);
    }

    if (strlen(upperKeyString) > 1) {
        return SDL_GetKeyFromName(upperKeyString);
    }
    
    return SDLK_UNKNOWN;
}

int read_moonlight_config(void)
{
    char *last_slash = strrchr(moon.cfg_path, '/');
    struct json_object *jfile = NULL;
    
    getcwd(moon.cfg_path, sizeof(moon.cfg_path));

    if (last_slash) *last_slash = '\0';

    strcat(moon.cfg_path, "/");
    strcat(moon.cfg_path, MOON_CFG_PATH);

    jfile = json_object_from_file(moon.cfg_path);
    if (jfile == NULL) {
        printf("Failed to read settings from json file (%s)\n", moon.cfg_path);
        return -1;
    }
    
    cfgReadCustomKeys(jfile);
    cfgReadClock(jfile);
    cfgReadMouse(jfile);

    json_object_put(jfile);
    return 0;
}

int write_moonlight_config(void)
{
    struct json_object *jfile = NULL;

    jfile = json_object_from_file(moon.cfg_path);
    if (jfile == NULL) {
        printf("Failed to write settings to json file (%s)\n", moon.cfg_path);
        return -1;
    }

    json_object_object_add(jfile, "A", json_object_new_string(SDL_GetKeyName(moon.customkey.A)));
    json_object_object_add(jfile, "B", json_object_new_string(SDL_GetKeyName(moon.customkey.B)));
    json_object_object_add(jfile, "X", json_object_new_string(SDL_GetKeyName(moon.customkey.X)));
    json_object_object_add(jfile, "Y", json_object_new_string(SDL_GetKeyName(moon.customkey.Y)));
    json_object_object_add(jfile, "L1", json_object_new_string(SDL_GetKeyName(moon.customkey.L1)));
    json_object_object_add(jfile, "L2", json_object_new_string(SDL_GetKeyName(moon.customkey.L2)));
    json_object_object_add(jfile, "R1", json_object_new_string(SDL_GetKeyName(moon.customkey.R1)));
    json_object_object_add(jfile, "R2", json_object_new_string(SDL_GetKeyName(moon.customkey.R2)));
    json_object_object_add(jfile, "LeftDpad", json_object_new_string(SDL_GetKeyName(moon.customkey.LeftDpad)));
    json_object_object_add(jfile, "RightDpad", json_object_new_string(SDL_GetKeyName(moon.customkey.RightDpad)));
    json_object_object_add(jfile, "UpDpad", json_object_new_string(SDL_GetKeyName(moon.customkey.UpDpad)));
    json_object_object_add(jfile, "DownDpad", json_object_new_string(SDL_GetKeyName(moon.customkey.DownDpad)));
    json_object_object_add(jfile, "Start", json_object_new_string(SDL_GetKeyName(moon.customkey.Start)));
    json_object_object_add(jfile, "Select", json_object_new_string(SDL_GetKeyName(moon.customkey.Select)));
    json_object_object_add(jfile, "Menu", json_object_new_string(SDL_GetKeyName(moon.customkey.Menu)));

    json_object_to_file(moon.cfg_path, jfile);
    json_object_put(jfile);
    printf("Writing settings to json file !\n");
    return 0;
}