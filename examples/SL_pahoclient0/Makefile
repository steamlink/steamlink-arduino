EXAMPLE_NAME = SL_pahoclient0

CPP = g++

GENERAL_CFLAGS = -c
SL_CFLAGS = -DUNIX

CPPFLAGS = $(GENERAL_CFLAGS) $(SL_CFLAGS)

BUILD_DIR = build
SL_SRC_PATH = ../..

PAHO_PATH = ../../paho

PAHO_C_SRC_PATH = $(PAHO_PATH)/MQTTPacket/src

PAHO_C_SRC_LIST = \
$(PAHO_C_SRC_PATH)/MQTTPacket.c \
$(PAHO_C_SRC_PATH)/MQTTDeserializePublish.c \
$(PAHO_C_SRC_PATH)/MQTTConnectClient.c \
$(PAHO_C_SRC_PATH)/MQTTSubscribeClient.c \
$(PAHO_C_SRC_PATH)/MQTTSerializePublish.c \
$(PAHO_C_SRC_PATH)/MQTTUnsubscribeClient.c 

INCLUDES = -I$(SL_SRC_PATH) -I$(PAHO_PATH)/MQTTClient/src -I$(PAHO_PATH)/MQTTPacket/src -I$(PAHO_PATH)/MQTTClient/src/linux

SL_SRC_LIST = \
	$(SL_SRC_PATH)/SteamLink.cpp \
	$(SL_SRC_PATH)/SteamLinkGeneric.cpp \
	$(SL_SRC_PATH)/SL_RingBuff.cpp \
	$(SL_SRC_PATH)/SteamLinkPaho.cpp

SL_OBJ_LIST = $(SL_SRC_LIST:$(SL_SRC_PATH)/%.cpp=$(BUILD_DIR)/%.o)
PAHO_C_OBJ_LIST = $(PAHO_C_SRC_LIST:$(PAHO_C_SRC_PATH)/%.c=$(BUILD_DIR)/%.o)

all: build
	$(BUILD_DIR)/$(EXAMPLE_NAME)

build: clean $(SL_OBJ_LIST) $(PAHO_C_OBJ_LIST)
	$(CPP) $(SL_CFLAGS) $(INCLUDES) $(SL_OBJ_LIST) $(PAHO_C_OBJ_LIST) $(EXAMPLE_NAME).cpp  -o $(BUILD_DIR)/$(EXAMPLE_NAME)

clean:
	rm -rf $(BUILD_DIR)
	mkdir $(BUILD_DIR)

$(BUILD_DIR)/%.o: $(SL_SRC_PATH)/%.cpp
	$(CPP) $(GENERAL_CFLAGS) $(SL_CFLAGS) $(INCLUDES) $< -o $@

$(BUILD_DIR)/%.o: $(PAHO_C_SRC_PATH)/%.c
	$(CPP) $(GENERAL_CFLAGS) $(SL_CFLAGS) $(INCLUDES) $< -o $@

