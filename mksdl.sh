#!/bin/sh

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' 

echo -e "${GREEN}====================================="
echo -e " Toolchain Setup for Miyoo Mini"
echo -e " This build environment relies on Stewards toolchain."
echo -e "=====================================${NC}\n"

ORIGINAL_PATH=$PATH

if [ ! -f "toolchain.tar.gz" ]; then
    echo -e "${YELLOW}toolchain.tar.gz not found, we need to download the toolchain now - it's around 800mb so sit back and relax${NC}"
    wget https://github.com/steward-fu/archives/releases/download/miyoo-mini/toolchain.tar.gz
elif [ ! -d "mmiyoo" ] && [ ! -d "prebuilt" ]; then
    echo -e "${GREEN}Extracting toolchain${NC}"
    tar xvf toolchain.tar.gz
fi

if [ -d "mmiyoo" ]; then
    echo -e "${GREEN}Copying miyoo dir to /opt${NC}"
    cp -r mmiyoo /opt
elif [ ! -d "/opt/mmiyoo" ]; then
    echo -e "${RED}mmiyoo folder not found in current directory or /opt.${NC}"
    return
fi

if [ -d "prebuilt" ]; then
    echo -e "${GREEN}Copying prebuilt dir to /opt${NC}"
    cp -r prebuilt /opt
elif [ ! -d "/opt/prebuilt" ]; then
    echo -e "${RED}prebuilt folder not found in current directory or /opt.${NC}"
fi

export PATH=/opt/mmiyoo/bin/:$PATH

echo -e "${YELLOW}Configuring environment...${NC}"
cd src
chmod -R a+w+x *
./run.sh config

export PATH=$ORIGINAL_PATH

echo -e "${GREEN}Script completed successfully. You're now ready to make (or reconfigure)${NC}"

echo -e "${YELLOW}\nPlease choose your next action:${NC}"
options=("Make now" "Exit script to workspace")
select opt in "${options[@]}"
do
    case $opt in
        "Make now")
            echo -e "${GREEN}Starting build process...${NC}"
            make
            break
            ;;
        "Exit script to workspace")
            echo -e "${GREEN}Exiting to workspace...${NC}"
            break
            ;;
        *) echo -e "${RED}Invalid option $REPLY${NC}";;
    esac
done
