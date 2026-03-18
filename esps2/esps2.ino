#include <Bluepad32.h>

#define PS2_CMD 23
#define PS2_DATA 19
#define PS2_CLK 18
#define PS2_ATT 5
#define PS2_ACK 21

GamepadPtr gamepads[BP32_MAX_GAMEPADS];

uint16_t ps2_buttons = 0xFFFF;
uint8_t ps2_lx = 127;
uint8_t ps2_ly = 127;
uint8_t ps2_rx = 127;
uint8_t ps2_ry = 127;

void onConnectedGamepad(GamepadPtr gp) {
  bool foundEmptySlot = false;
    Serial.println("Controle conectado!");
    for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
        if (gamepads[i] == nullptr) {
          Serial.printf("CALLBACK: Controller is connected, index=%d\n", i);
          ControllerProperties properties = gp->getProperties();
          Serial.printf("Controller model: %s, VID=0x%04x, PID=0x%04x\n", gp->getModelName().c_str(), properties.vendor_id, properties.product_id);
          gamepads[i] = gp;
          break;
        }
    }
    if (!foundEmptySlot) {
        Serial.println("CALLBACK: Controller connected, but could not found empty slot");
    }
}

void onDisconnectedGamepad(GamepadPtr gp) {
    Serial.println("Controle desconectado!");
}

uint8_t convertAxis(int v) {
    return (v + 512) / 4;
}

void mapXboxToPS2(GamepadPtr gp) {

    ps2_buttons = 0xFFFF;

    if (gp->a()) ps2_buttons &= ~(1 << 14);
    if (gp->b()) ps2_buttons &= ~(1 << 13);
    if (gp->x()) ps2_buttons &= ~(1 << 15);
    if (gp->y()) ps2_buttons &= ~(1 << 12);

    if (gp->l1()) ps2_buttons &= ~(1 << 10);
    if (gp->r1()) ps2_buttons &= ~(1 << 11);

    if (gp->miscStart()) ps2_buttons &= ~(1 << 3);
    if (gp->miscSelect()) ps2_buttons &= ~(1 << 0);

    ps2_lx = convertAxis(gp->axisX());
    ps2_ly = convertAxis(gp->axisY());
    ps2_rx = convertAxis(gp->axisRX());
    ps2_ry = convertAxis(gp->axisRY());
}

uint8_t ps2_buffer[9];

void preparePS2Packet() {

    ps2_buffer[0] = 0xFF;
    ps2_buffer[1] = 0x73;
    ps2_buffer[2] = 0x5A;

    ps2_buffer[3] = ps2_buttons & 0xFF;
    ps2_buffer[4] = (ps2_buttons >> 8) & 0xFF;

    ps2_buffer[5] = ps2_rx;
    ps2_buffer[6] = ps2_ry;
    ps2_buffer[7] = ps2_lx;
    ps2_buffer[8] = ps2_ly;
}

uint8_t readCMD() {
    return digitalRead(PS2_CMD);
}

void writeDATA(uint8_t v) {
    digitalWrite(PS2_DATA, v);
}

void ps2Task() {

    if (digitalRead(PS2_ATT) == LOW) {

        preparePS2Packet();

        for (int i = 0; i < 9; i++) {

            uint8_t out = ps2_buffer[i];

            for (int b = 0; b < 8; b++) {

                while (digitalRead(PS2_CLK) == HIGH);

                writeDATA(out & 1);
                out >>= 1;

                while (digitalRead(PS2_CLK) == LOW);
            }
        }
    }
}

void debugController(GamepadPtr gp) {

    Serial.print("LX:");
    Serial.print(gp->axisX());

    Serial.print(" LY:");
    Serial.print(gp->axisY());

    Serial.print(" RX:");
    Serial.print(gp->axisRX());

    Serial.print(" RY:");
    Serial.print(gp->axisRY());

    Serial.print(" BTN:");
    Serial.print(gp->buttons(), BIN);

    Serial.print(" MISC:");
    Serial.println(gp->miscButtons(), BIN);
}

void setup() {

    Serial.begin(115200);

    pinMode(PS2_CMD, INPUT);
    pinMode(PS2_CLK, INPUT);
    pinMode(PS2_ATT, INPUT);

    pinMode(PS2_DATA, OUTPUT);
    pinMode(PS2_ACK, OUTPUT);

    digitalWrite(PS2_DATA, HIGH);

    BP32.setup(&onConnectedGamepad, &onDisconnectedGamepad);
    BP32.forgetBluetoothKeys();
    BP32.enableVirtualDevice(false);

    Serial.println("Adaptador Xbox → PS2 pronto");
}

void loop() {

    bool dataUpdated = BP32.update();

    if (dataUpdated) {
      for (auto gp : gamepads) {
          if (gp && gp->isConnected()) {
              mapXboxToPS2(gp);
              debugController(gp);
          }
      }
    }

    //ps2Task();
}