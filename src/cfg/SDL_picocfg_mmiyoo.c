#include <string.h>
#include <stdio.h> 
#include <unistd.h>
#include <json-c/json.h>

#include <SDL2/SDL.h>
#include "../../cfg/SDL_picocfg_mmiyoo.h"

PICO pico = {0};
int cpuclock = 0;

int cfgReadMouse(struct json_object *jfile) {
    struct json_object *jval = NULL;
    struct json_object *jScaleFactor = NULL;
    struct json_object *jAcceleration = NULL;
    struct json_object *jAccelerationRate = NULL;
    struct json_object *jMaxAcceleration = NULL;
    struct json_object *jIncrementModifier = NULL;
    json_object_object_get_ex(jfile, "mouse", &jval);

    if (jval) {
        json_object_object_get_ex(jval, "scaleFactor", &jScaleFactor);
        if (jScaleFactor) {
            pico.mouse.scaleFactor = json_object_get_int(jScaleFactor);
        } else {
            pico.mouse.scaleFactor = MMIYOO_DEFAULT_SCALE_FACTOR;
        }
        
        json_object_object_get_ex(jval, "acceleration", &jAcceleration);
        if (jAcceleration) {
            pico.mouse.acceleration = json_object_get_double(jAcceleration);
        } else {
            pico.mouse.acceleration = MMIYOO_DEFAULT_ACCELERATION;
        }

        json_object_object_get_ex(jval, "accelerationRate", &jAccelerationRate);
        if (jAccelerationRate) {
            pico.mouse.accelerationRate = json_object_get_double(jAccelerationRate);
        } else {
            pico.mouse.accelerationRate = MMIYOO_DEFAULT_ACCELERATION_RATE;
        }

        json_object_object_get_ex(jval, "maxAcceleration", &jMaxAcceleration);
        if (jMaxAcceleration) {
            pico.mouse.maxAcceleration = json_object_get_double(jMaxAcceleration);
        } else {
            pico.mouse.maxAcceleration = MMIYOO_DEFAULT_MAX_ACCELERATION;
        }

        json_object_object_get_ex(jval, "incrementModifier", &jIncrementModifier);
        if (jIncrementModifier) {
            pico.mouse.incrementModifier = json_object_get_int(jIncrementModifier);
        } else {
            pico.mouse.incrementModifier = MMIYOO_DEFAULT_INCREMENT_MODIFIER;
        }

        printf("[json] Mouse settings: scaleFactor=%d, acceleration=%f, accelerationRate=%f, maxAcceleration=%f, incrementModifier=%f\n",
               pico.mouse.scaleFactor, pico.mouse.acceleration, pico.mouse.accelerationRate, pico.mouse.maxAcceleration, pico.mouse.incrementModifier);
    } else {
        printf("Mouse settings not found in json file. Using defaults.\n");
        pico.mouse.scaleFactor = MMIYOO_DEFAULT_SCALE_FACTOR;
        pico.mouse.acceleration = MMIYOO_DEFAULT_ACCELERATION;
        pico.mouse.accelerationRate = MMIYOO_DEFAULT_ACCELERATION_RATE;
        pico.mouse.maxAcceleration = MMIYOO_DEFAULT_MAX_ACCELERATION;
        pico.mouse.incrementModifier = MMIYOO_DEFAULT_INCREMENT_MODIFIER;
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
            if (cpuclock > MMIYOO_MAX_CPU_CLOCK) {
                printf("[json] pico.cpuclock too high! Using default: %d\n", MMIYOO_DEFAULT_CPU_CLOCK);
                pico.cpuclock = MMIYOO_DEFAULT_CPU_CLOCK;
            } else if (cpuclock < MMIYOO_MIN_CPU_CLOCK) {
                printf("[json] pico.cpuclock too low! Using default: %d\n", MMIYOO_DEFAULT_CPU_CLOCK);
                pico.cpuclock = MMIYOO_DEFAULT_CPU_CLOCK;
            } else {
                printf("[json] pico.cpuclock: %d\n", cpuclock);
                pico.cpuclock = cpuclock;
            }
        } else {
            printf("Invalid pico.cpuclock value. Using default: %d\n", MMIYOO_DEFAULT_CPU_CLOCK);
            pico.cpuclock = MMIYOO_DEFAULT_CPU_CLOCK;
        }
    } else {
        printf("pico.cpuclock not found in json file. Using default: %d.\n", MMIYOO_DEFAULT_CPU_CLOCK);
        pico.cpuclock = MMIYOO_DEFAULT_CPU_CLOCK;
    }

    json_object_object_get_ex(jfile, "cpuclockincrement", &jval);
    if (jval) {
        const char *cpuclockincrement_str = json_object_get_string(jval);
        if (cpuclockincrement_str) {
            int cpuclockincrement = atoi(cpuclockincrement_str);
            if (cpuclockincrement > MMIYOO_MAX_CPU_CLOCK_INCREMENT) {
                printf("[json] pico.cpuclockincrement too high! Using maximum allowed: 100\n");
                pico.cpuclockincrement = MMIYOO_MAX_CPU_CLOCK_INCREMENT;
            } else {
                printf("[json] pico.cpuclockincrement: %d\n", cpuclockincrement);
                pico.cpuclockincrement = cpuclockincrement;
            }
        } else {
            printf("Invalid pico.cpuclockincrement value. Using default: %d\n", MMIYOO_DEFAULT_CPU_CLOCK_INCREMENT);
            pico.cpuclockincrement = MMIYOO_DEFAULT_CPU_CLOCK_INCREMENT;
        }
    } else {
        printf("pico.cpuclockincrement not found in json file. Using default: %d.\n", MMIYOO_DEFAULT_CPU_CLOCK_INCREMENT);
        pico.cpuclockincrement = MMIYOO_DEFAULT_CPU_CLOCK_INCREMENT;
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
            pico.customkey.A = (jval) ? received_keycode : default_keycode;
        } else if (strcmp(keys[i], "B") == 0) {
            pico.customkey.B = (jval) ? received_keycode : default_keycode;
        } else if (strcmp(keys[i], "X") == 0) {
            pico.customkey.X = (jval) ? received_keycode : default_keycode;
        } else if (strcmp(keys[i], "Y") == 0) {
            pico.customkey.Y = (jval) ? received_keycode : default_keycode;
        } else if (strcmp(keys[i], "L1") == 0) {
            pico.customkey.L1 = (jval) ? received_keycode : default_keycode;
        } else if (strcmp(keys[i], "L2") == 0) {
            pico.customkey.L2 = (jval) ? received_keycode : default_keycode;
        } else if (strcmp(keys[i], "R1") == 0) {
            pico.customkey.R1 = (jval) ? received_keycode : default_keycode;
        } else if (strcmp(keys[i], "R2") == 0) {
            pico.customkey.R2 = (jval) ? received_keycode : default_keycode;
        } else if (strcmp(keys[i], "LeftDpad") == 0) {
            pico.customkey.LeftDpad = (jval) ? received_keycode : default_keycode;
        } else if (strcmp(keys[i], "RightDpad") == 0) {
            pico.customkey.RightDpad = (jval) ? received_keycode : default_keycode;
        } else if (strcmp(keys[i], "UpDpad") == 0) {
            pico.customkey.UpDpad = (jval) ? received_keycode : default_keycode;
        } else if (strcmp(keys[i], "DownDpad") == 0) {
            pico.customkey.DownDpad = (jval) ? received_keycode : default_keycode;
        } else if (strcmp(keys[i], "Start") == 0) {
            pico.customkey.Start = (jval) ? received_keycode : default_keycode;
        } else if (strcmp(keys[i], "Select") == 0) {
            pico.customkey.Select = (jval) ? received_keycode : default_keycode;
        } else if (strcmp(keys[i], "Menu") == 0) {
            pico.customkey.Menu = (jval) ? received_keycode : default_keycode;
        }
        
        if (jval) {
            if (received_keycode != SDLK_UNKNOWN) {
                printf("[json] pico.customkey.%s: %d\n", keys[i], received_keycode);
            } else {
                printf("Invalid pico.customkey.%s, reset as %d\n", keys[i], default_keycode);
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

int read_pico_config(void)
{
    char *last_slash = strrchr(pico.cfg_path, '/');
    struct json_object *jfile = NULL;
    
    getcwd(pico.cfg_path, sizeof(pico.cfg_path));

    if (last_slash) *last_slash = '\0';

    strcat(pico.cfg_path, "/");
    strcat(pico.cfg_path, PICO_CFG_PATH);

    jfile = json_object_from_file(pico.cfg_path);
    if (jfile == NULL) {
        printf("Failed to read settings from json file (%s)\n", pico.cfg_path);
        return -1;
    }
    
    cfgReadCustomKeys(jfile);
    cfgReadClock(jfile);
    cfgReadMouse(jfile);

    json_object_put(jfile);
    return 0;
}

int write_pico_config(void) {
    struct json_object *jfile = NULL;
    struct json_object *jmouse = NULL;

    jfile = json_object_from_file(pico.cfg_path);

    if (jfile == NULL) {
        jfile = json_object_new_object();
        if (jfile == NULL) {
            printf("Failed to create json object\n");
            return -1;
        }
    }

     // custom keys but don't write these as they shouldn't change
    // jcustomkeys = json_object_new_object();
    // json_object_object_add(jfile, "A", json_object_new_string(SDL_GetKeyName(pico.customkey.A)));
    // json_object_object_add(jfile, "B", json_object_new_string(SDL_GetKeyName(pico.customkey.B)));
    // json_object_object_add(jfile, "X", json_object_new_string(SDL_GetKeyName(pico.customkey.X)));
    // json_object_object_add(jfile, "Y", json_object_new_string(SDL_GetKeyName(pico.customkey.Y)));
    // json_object_object_add(jfile, "L1", json_object_new_string(SDL_GetKeyName(pico.customkey.L1)));
    // json_object_object_add(jfile, "L2", json_object_new_string(SDL_GetKeyName(pico.customkey.L2)));
    // json_object_object_add(jfile, "R1", json_object_new_string(SDL_GetKeyName(pico.customkey.R1)));
    // json_object_object_add(jfile, "R2", json_object_new_string(SDL_GetKeyName(pico.customkey.R2)));
    // json_object_object_add(jfile, "LeftDpad", json_object_new_string(SDL_GetKeyName(pico.customkey.LeftDpad)));
    // json_object_object_add(jfile, "RightDpad", json_object_new_string(SDL_GetKeyName(pico.customkey.RightDpad)));
    // json_object_object_add(jfile, "UpDpad", json_object_new_string(SDL_GetKeyName(pico.customkey.UpDpad)));
    // json_object_object_add(jfile, "DownDpad", json_object_new_string(SDL_GetKeyName(pico.customkey.DownDpad)));
    // json_object_object_add(jfile, "Start", json_object_new_string(SDL_GetKeyName(pico.customkey.Start)));
    // json_object_object_add(jfile, "Select", json_object_new_string(SDL_GetKeyName(pico.customkey.Select)));
    // json_object_object_add(jfile, "Menu", json_object_new_string(SDL_GetKeyName(pico.customkey.Menu)));
    // json_object_object_add(jfile, "customkeys", jcustomkeys);

    jmouse = json_object_new_object();
    json_object_object_add(jmouse, "scaleFactor", json_object_new_int(pico.mouse.scaleFactor));
    json_object_object_add(jmouse, "acceleration", json_object_new_double(pico.mouse.acceleration));
    json_object_object_add(jmouse, "accelerationRate", json_object_new_double(pico.mouse.accelerationRate));
    json_object_object_add(jmouse, "maxAcceleration", json_object_new_double(pico.mouse.maxAcceleration));
    json_object_object_add(jmouse, "incrementModifier", json_object_new_double(pico.mouse.incrementModifier));
    json_object_object_add(jfile, "mouse", jmouse);

    json_object_object_add(jfile, "cpuclock", json_object_new_int(pico.cpuclock));
    json_object_object_add(jfile, "cpuclockincrement", json_object_new_int(pico.cpuclockincrement));

    if (json_object_to_file(pico.cfg_path, jfile) != 0) {
        printf("Failed to write settings to json file (%s)\n", pico.cfg_path);
        json_object_put(jfile);
        return -1;
    }

    json_object_put(jfile);
    printf("Updated settings in json file (%s) successfully!\n", pico.cfg_path);
    return 0;
}