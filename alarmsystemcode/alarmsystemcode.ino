//definire valori prestabilite pentru pini
#define KB_DATA 5
#define KB_CLOCK 6
#define LED_R 7
#define LED_G 8
#define LED_B 9
#define SPEAKER_PIN 10
#define ECHO_PIN 11
#define TRIGGER_PIN 12
#define LIGHT_SENSOR_OUT 14 //pin-ul A0 este pin-ul 14
#define PREDEFINED_CODE 1234 //aceasta este valoarea prestabilita a codului
#define LIGHT_MAX_VALUE 20 //valoarea maxima a senzorului de lumina pentru care se considera noapte
#define DISTANCE_MAX_VALUE 15 //valoarea maxima de distanta pana la care suna alarma
#define DISTANCE_WARNING_VALUE 35 //valoarea maxima de distanta pana la care se avertizeaza utilizatorul


//clasa de baza pentru senzori
class Sensor {
protected:
    //am considerat ca orice senzor are un pin, o valoare citita si o valoare la care declanseaza un eveniment
    int value, sensor_pin, trigger_value;
public:
    Sensor(int sensor_pin, int trigger_value) {
        this->sensor_pin = sensor_pin;
        this->trigger_value = trigger_value;
    }
    int getValue() { //aceasta functie returneaza ultima valoare citita
        return value;
    }
    int getSensorPin() { //aceasta functie intoarce pin-ul senzorului
        return sensor_pin;
    }
    int getTriggerValue() { //aceasta functie intoarce valoarea de declansare eveniment
        return trigger_value;
    }
};

//clasa pentru senzorul de lumina (derivata din clasa Sensor)
class LightSensor : public Sensor {
public:
    LightSensor(int sensor_pin, int trigger_value) : Sensor(sensor_pin, trigger_value) {
        value = readValue();
    }
    int readValue() { //se suprascrie functia de citire a valorii
        int light;
        light = analogRead(sensor_pin);
        //in plus fata de functia originala, se afiseaza pe serial monitor
        Serial.print("Light: "); Serial.println(light);
        return light;
    }
    //functia isItNight intoarce 1 daca valoarea citita de senzor se incadreaza in limita
    //pana la care se considera noapte, altfel 0
    int isItNight() {
        if (readValue() <= trigger_value) return 1;
        return 0;
    }
};


//clasa pentru senzorul de distanta (derivata din clasa Sensor)
class DistanceSensor : public Sensor {
    int echo_pin; //senzorul de distanta are un pin in plus, echo_pin
    int warning_value; //o alta caracteristica a acestuia este valoarea de avertizare
public:
    DistanceSensor(int trigger_pin, int echo_pin, int trigger_value, int warning_value) : Sensor(trigger_pin, trigger_value) {
        this->echo_pin = echo_pin;
        this->warning_value = warning_value;
        value = readValue();
    }
    //functia readValue citeste valoarea senzorului
    int readValue() {
        int duration, distance;
        digitalWrite(sensor_pin, LOW); //se scrie LOW pe pin-ul de intrare al senzorului
        delayMicroseconds(2); //se asteapta 2us
        digitalWrite(sensor_pin, HIGH); //se scrie HIGH pe pin (moment in care senzorul emite un semnal)
        delayMicroseconds(10); //se emite semnalul timp de 10us
        digitalWrite(sensor_pin, LOW); //se opreste emisia (se scrie LOW pe pin-ul de intrare)
        duration = pulseIn(echo_pin, HIGH); //durata pana la intoarcerea sunetului se citeste ca un puls pe pin-ul echo_pin
        distance = duration * 0.034 / 2; //distanta se calculeaza in functie de viteza luminii
        Serial.print("Distance: "); Serial.println(distance);
        return distance; //se intoarce distanta calculata
    }
    //functia isItTooClose intoarce 1 daca un obstacol este prea aproape de zenzor si trebuie avertizat
    int isItTooClose() {
        if (readValue() <= warning_value) return 1;
        return 0;
    }
    //functia isItTriggered intoarce 1 daca valoarea data de senzor e suficient de mica pentru declansarea alarmei
    int isItTriggered() {
        if (readValue() <= trigger_value) return 1;
        return 0;
    }
};

//clasa de baza pentru dispozitivele de iesire
class OutputDevice {
protected:
    int pin; //toate dispozitivele de iesire au un pin de iesire
public:
    OutputDevice(int pin) {
        this->pin = pin;
    }
    //functie implicita pentru output digital
    virtual void output(int val) {
        digitalWrite(pin, val);
    }
    //functie implicita pentru output analog
    virtual void analogOutput(int val) {
        analogWrite(pin, val);
    }
};

//clasa pentru difuzor/buzzer (derivata din OutputDevice)
class Speaker : public OutputDevice {
public:
    Speaker(int speaker_pin) : OutputDevice(speaker_pin) {}
    //functia de iesire rescrisa
    void output(int val) {
        if (val == 0) digitalWrite(pin, LOW);
        else digitalWrite(pin, HIGH);
    }
    //functia pentru oprire a difuzorului
    void off (){
        output(0);
    }
    //functia pentru emiterea unui sunet atunci cand se apasa o tasta
    void keyPressed() {
        for (int i = 0; i < 10; i++) {
            output(1);
            delay(10);
            output(0);
            delay(10);
        }
    }
    //functia pentru a emite semnal sonor de avertizare daca e introdus un cod gresit
    void wrongCodeEntered() {
        //se emite de 3 ori un semnal de 100ms cu perioada 2ms, cu pauza 100ms intre semnale
        for (int k = 0; k < 2; k++) {
            for (int i = 0; i < 50; i++) {
                output(1);
                delay(1);
                output(0);
                delay(1);
            }
            delay(100);
        }
    }
    //functia pentru alarma (emite un semnal cu perioada 2*del)
    void alarm(int del) {
        output(1);
        delay(del);
        output(0);
        delay(del);
    }
};

//clasa pentru led (derivata din OutputDevice)
class Led : public OutputDevice {
public:
    Led(int led_pin) : OutputDevice(led_pin) {}
    //rescrierea functiei de optput
    void output(int val) {
        if (val == 0) digitalWrite(pin, LOW);
        else digitalWrite(pin, HIGH);
    }
    //rescrierea functiei de output analogic
    void analogOutput(int val) {
        if (val < 0) val = 0;
        else if (val > 255) val = 255;
        analogWrite(pin, val);
    }
};

//clasa pentru led RGB
class RGBLed {
    //led-ul RGB are 3 intrari pentru fiecare dintre culorile de baza
    //asa ca am considerat led-ul RGB ca fiind format din 3 led-uri (r,g,b)
    Led *r, *g, *b;
public:
    RGBLed(int red_pin, int green_pin, int blue_pin) {
        //in constructor se initializeaza cele 3 componente ale led-ului pentru pinii corespunzatori
        r = new Led(red_pin);
        g = new Led(green_pin);
        b = new Led(blue_pin);
    }
    off() {
        //se opreste led-ul RGB (0 pe toate iesirile)
        r->output(0);
        g->output(0);
        b->output(0);
    }
    red() {
        //se porneste doar "componenta" rosie
        r->output(1);
        g->output(0);
        b->output(0);
    }
    green() {
        //se porneste doar "componenta" verde
        r->output(0);
        g->output(1);
        b->output(0);
    }
    blue() {
        //se porneste doar "componenta" albastra
        r->output(0);
        g->output(0);
        b->output(1);
    }
    yellow() {
        //pentru galben, se seteaza HIGH pe R si G (combinate rezultand galben)
        r->output(1);
        g->output(1);
        b->output(0);
    }
};

//se declara si initializeaza un obiect de tip Speaker (se va folosi in clasa pentru tastatura)
Speaker *speaker = new Speaker(SPEAKER_PIN);

//clasa pentru tastatura capacitiva
class CapacitiveKeyboard {
    //se folosesc pinii pentru data si clock
    int data_pin, clock_pin;
public:
    CapacitiveKeyboard(int data_pin, int clock_pin) {
        this->data_pin = data_pin;
        this->clock_pin = clock_pin;
    }
    //functia ce se ocupa de preluarea datelor de intrare
    int getInput() {
        int databit; //in variabila databit se va stoca 0 daca butonul e apasat, altfel 1
        //se verifica, pe rand, fiecare dintre cele 17 butoane
        for (int button = 1; button < 17; button++)
        {
            digitalWrite(clock_pin, LOW); //pe pin-ul de clock se scrie valoarea LOW
            databit = digitalRead(data_pin); //se citeste valoarea de pe pinul de date
            digitalWrite(clock_pin, HIGH); //se scrie HIGH pe pin-ul de clock
            if (databit == 0) { //daca databit are valoarea 0 (butonul e apasat)
                speaker->keyPressed(); //difuzorul face un sunet, semn ca a preluat tasta
                Serial.print("Keyboard: "); Serial.println(button);
                delay(200); //delay 200ms pentru a evita preluarea unei dubluri
                return button; //se intoarce numarul butonului ce a fost apasat
            }
        }
        return 0; //daca nu s-a apasat niciun buton, se intoarce 0
    }
};

//se declara si initializeaza un obiect de tip DistanceSensor (va fi folosit in clasa AlarmSystem)
DistanceSensor *distance_sensor = new DistanceSensor(TRIGGER_PIN, ECHO_PIN, DISTANCE_MAX_VALUE, DISTANCE_WARNING_VALUE);

class AlarmSystem {
    int system_state; // on/off (1/0)
    int triggered; // 1 daca s-a declansat alarma, altfel 0
    int warn; //1 daca se avertizeaza doar prin semnal luminos
    int current_code; //variabila ce stocheaza codul curent tastat de utilizator
    int current_code_digits; //variabila ce stocheaza numarul de cifre al codului tastat
    int system_code; //codul sistemului de alarma
    int system_code_digits; //lungimea codului sistemului
public:
    AlarmSystem(int code) {
        //codul sistemului va fi primit prin parametrul code
        this->system_code = code;
        system_code_digits = 0; //lungimea acestuia e initializata cu 0
        //se incremenetaza lungimea pentru fiecare cifra a codului
        while (code) {
            system_code_digits++;
            code = code / 10;
        }
        //implicit, sistemul e oprit, nedeclansat, nu avertizeaza si nu s-a introdus niciun cod
        system_state = 0;
        triggered = 0;
        warn = 0;
        current_code_digits = 0;
        current_code = 0;
    }
    //functia switchSystemState opreste sistemul daca e pornit si vice-versa
    void switchSystemState() {
        if (system_state) system_state = 0;
        else system_state = 1;
        current_code = 0; //se reseteaza codul introdus
        current_code_digits = 0; //se reseteaza numarul de cifre al codului introdus
    }
    //getSystemState intoarce starea sistemului (1 - pornit; 0 - oprit)
    int getSystemState() {
        return system_state;
    }
    //getWarning returneaza 1 daca sistemul trebuie sa avertizeze luminos, 0 altfel
    int getWarning() {
        warn = distance_sensor->isItTooClose();
        return warn;
    }
    //getTriggered returneaza 1 daca alarma e declansata, altfel 0
    int getTriggered() {
        triggered = distance_sensor->isItTriggered();
        return triggered;
    }
    /*functia checkCode preia ultima tasta apasata de utilizator ca parametru
    si verifica daca codul introdus coincide cu codul sistemului*/
    void checkCode(int key) {
        //daca tasta apasata e de la 0 la 9
        if (key < 10) {
            //se inmulteste codul curent cu 10 si se adauga tasta
            current_code = current_code * 10 + key;
            //se incrementeaza cu 1 numarul de cifre
            current_code_digits++;
        }
        //altfel, daca e o tasta de la 10 la 17
        else {
            //se inmulteste codul cu 100 si se adauga tasta
            current_code = current_code * 100 + key;
            //se adauga 2 la numarul de cifre
            current_code_digits += 2;
        }
        /*daca codul introdus are lungimea codului sistemului, dar difera de acesta
        atunci inseamna ca s-a introdus un cod gresit*/
        if ((current_code_digits >= system_code_digits) && (current_code != system_code)) {
            speaker->wrongCodeEntered(); //difuzorul emite un semnal ce anunta cod gresit
            current_code_digits = 0; //se reseteaza codul curent
            current_code = 0; //se reseteaza numarul de cifre al codului
        }
        //daca codul coincide cu codul sistemului, atunci se schimba starea acestuia
        if (current_code == system_code) switchSystemState();
    }
};

//se declara si initialiezeaza obiecte de tip RGBLed, CapacitiveKeyboard, LightSensor si AlarmSystem
RGBLed *rgbled = new RGBLed(LED_R, LED_G, LED_B);
CapacitiveKeyboard *keyboard = new CapacitiveKeyboard(KB_DATA, KB_CLOCK);
AlarmSystem *alarm_system = new AlarmSystem(PREDEFINED_CODE);
LightSensor *light_sensor = new LightSensor(LIGHT_SENSOR_OUT, LIGHT_MAX_VALUE);

void setup() {
    //se initializeaza port-ul serial pentru serial monitor
    Serial.begin(9600);
    //se seteaza pinii pentru intrare/iesire, in functie de caz
    pinMode(TRIGGER_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);
    pinMode(LIGHT_SENSOR_OUT, INPUT);
    pinMode(SPEAKER_PIN, OUTPUT);
    pinMode(LED_R, OUTPUT);
    pinMode(LED_G, OUTPUT);
    pinMode(LED_B, OUTPUT);
    pinMode(KB_DATA, INPUT);
    pinMode(KB_CLOCK, OUTPUT);
}

long i = 0;

void loop() {
    int key = keyboard->getInput(); //se varifica la fiecare iteratie input-ul de la tastatura
    //daca key!=0 (adica s-a apasat o tasta), se apeleaza functia ce se ocupa cu prelucrarea codului
    if (key) alarm_system->checkCode(key);
    //daca sistemul este pornit sau e "noapte"
    if (alarm_system->getSystemState() || light_sensor->isItNight()) {
        //daca sistemul e declansat
        if (alarm_system->getTriggered()) {
            /*variabila i se foloseste pentru alternanta intre culorile galben/albastru
            a led-uli, respectiv alternanta "tonului" difuzorului*/
            if (i < 0 || i>150) i = 0; //daca depaseste intervalul, se reseteaza i la 0
            //daca i este intre 0 si 100
            if (i < 100) {
                rgbled->blue(); //led-ul este albastru
                speaker->alarm(1); //alarma suna cu o perioada de 2ms (1ms HIGH, 1ms LOW)
            }
            //daca i este intre 100 si 150
            else {
                rgbled->yellow(); //led-ul este gablen
                speaker->alarm(2); //alarma suna cu perioada de 4ms (2ms HIGH, 2ms LOW)
                /*obs: se alege un interval de 2 ori mai mic pentru i de aceasta data
                deoarece delay-ul pentru difuzor este de 2 ori mai mare si se urmareste
                ca perioada in care led-ul este galben sa fie egala cu cea in care e albastru*/
            }
            i++; //incrementeaza i
        }
        //altfel, daca sistemul nu e declansat, dar trebuie sa emita o avertizare
        else if (alarm_system->getWarning()) {
            //se repeta procedura din if-ul de mai sus, doar ca se aprinde led-ul doar
            if (i < 0 || i>200) i = 0;
            if (i < 100)
                rgbled->blue();
            else
                rgbled->yellow();
            delay(2); //delay de 2ms intre iteratiile lui i
            i++;
        }
        //altfel, daca sistemul nu e declansat si nu emite avertizare
        else {
            rgbled->red(); //led-ul e aprins rosu, indicand faptul ca e pornit
        }
    }
    //daca sistemul nu e pornit si e zi
    else {
        rgbled->green(); //led-ul e verde, indica faptul ca sistemul e oprit
        speaker->off(); //difuzorul e oprit
    }
}
