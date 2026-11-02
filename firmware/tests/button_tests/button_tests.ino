#define START_BUTTON 35

void setup() {
    Serial.begin(115200);
    pinMode(START_BUTTON, INPUT_PULLDOWN);
}

void loop() {
    Serial.print("GPIO16 = ");
    Serial.println(digitalRead(START_BUTTON));

    delay(200);
}