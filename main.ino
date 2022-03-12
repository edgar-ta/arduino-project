/** Lectura que indica que el sensor no está detectando nada **/
#define NULL_READING 0

/** 
 * Velocidad máxima (inclusiva) que un objeto puede tener
 * sin ser considerado peligroso (en metros / segundo)
 * */
#define EMERGENCY_SPEED 3.2

/**
 * Distancia mínima (inclusiva) que un objeto puede tener
 * sin ser considerado peligroso (en metros); un objeto
 * más cercano que esto es considerado peligroso
 * */
#define MODERATE_DISTANCE 1

/**
 * Direcciones en las que un objeto se mueve respecto
 * al sensor.
 * 
 * Hin -> (alemán): Que se está alejando
 * Her <- (alemán): Que se está acercando
 * Unknown (inglés): Dirección desconocida
 * */
typedef enum {
    HIN, HER, UNKNOWN
} Direction;


/**
 * Clase que monitorea datos de un sensor de distancia, calcula y actúa
 * según es necesario
 * 
 * @param pin El pin (número de pin) a utilizar para leer
 * datos del sensor de distancia
 * @param isActive Flag; indica si la conexión está recibiendo
 * y calculando datos o no. Cuando la conexión está inactiva, tiene
 * valores por defecto que son inválidos
 * @param distance La distancia anterior capturada por la conexión (metros)
 * @param speed La velocidad anterior capturada por la conexión (metros / segundo)
 * @param time El tiempo en que los datos anteriores fueron captados (milisegundos)
 * por la conexión
 * @param direction La dirección anterior capturada por la conexión
 * */
typedef struct {
    short pin;
    bool isActive;
    double distance, speed;
    unsigned long time;
    Direction direction;
} MotionConnection;

void onEmergency(MotionConnection* self) {
    // 
};

void onModerate(MotionConnection* self) {
    // 
};

void onSafe(MotionConnection* self) {
    // 
};

/**
 * Convierte una lectura a metros
 * @param reading Lectura del sensor (dada por digitalRead)
 * @returns Distancia (en metros) que corresponde a la lectura dada
 * */
double readingToDistance(int reading) {
    if (reading == NULL_READING) return NULL;
    return reading / 32;
};

/**
 * Inicializa una conexión
 * @param self La conección a inicializar
 * @param * Atributos de una conexión,
 * as described en su declaración
 * */
void MotionConnection_init(
    MotionConnection* self,
    short pin,
    bool isActive,
    double distance,
    double speed,
    unsigned long time,
    Direction direction
) {
    self->pin = pin;
    self->isActive = isActive;
    self->distance = distance;
    self->speed = speed;
    self->time = time;
    self->direction = direction;
};

/**
 * Crea una nueva conexión
 * @param pin El pin a utilizar para lecturas, as described
 * en la declaración de MotionConnection
 * @returns Puntero a la conexión
 * */
MotionConnection* MotionConnection_new(short pin) {
    MotionConnection* motionConnection = (MotionConnection*) malloc(sizeof(MotionConnection));
    MotionConnection_init(motionConnection, pin, false, NULL, NULL, NULL, UNKNOWN);
    pinMode(pin, INPUT);
    return motionConnection;
};

/**
 * @brief Inicia una conexión  
 * 
 * Guarda el tiempo actual (dado por la built-in millis),  
 * la distancia dada, y cambia el estado de la conexión a activo  
 * 
 * @param self La conexión to act on
 * @param distance La distancia
 * */
void MotionConnection_startConnection(MotionConnection* self, double distance) {
    self->distance = distance;
    self->time = millis();
    self->isActive = true;
};

/**
 * Calcula los nuevos datos de la conexión
 * */
void MotionConnection_continueConnection(MotionConnection* self, double distance) {
    unsigned long time = millis();
    double distanceDelta = self->distance - distance;
    unsigned long timeDelta = self->time - time;

    // the speed is measured in m/s, but the time delta was given in ms
    self->speed = 1000 * distanceDelta / timeDelta;
    self->direction = distanceDelta < 0? HIN: HER;
    self->time = time;
    self->distance = distance;
};

/**
 * Termina la conexión
 * 
 * ---
 * Si la conexión no está activa; solo regresa.
 * Si sí lo está, se inicializa con los valores por defecto
 * (como en el constructor), pero esta vez pasa su propio
 * pin en vez de solicitarlo
 * */
void MotionConnection_endConnection(MotionConnection* self) {
    if (self->isActive)
    MotionConnection_init(self, self->pin, true, NULL, NULL, NULL, UNKNOWN);
};

double MotionConnection_getDistance(MotionConnection* self) {
    int reading = digitalRead(self->pin);
    return readingToDistance(reading);
};

void MotionConnection_act(MotionConnection* self) {
    if (self->direction == HER && self->speed >= EMERGENCY_SPEED) return onEmergency(self);
    if (self->direction == HER || self->distance <= MODERATE_DISTANCE) return onModerate(self);
    onSafe(self);
};

void MotionConnection_listen(MotionConnection* self) {
    double distance = MotionConnection_getDistance(self);
    if (distance == NULL) {
        MotionConnection_endConnection(self);
        return;
    }
    if (self->isActive) MotionConnection_startConnection(self, distance);
    else MotionConnection_continueConnection(self, distance);
    MotionConnection_act(self);
};

const MotionConnection* mainConnection;

void setup() {
    mainConnection = MotionConnection_new(5);
};

void loop() {
    MotionConnection_listen((MotionConnection*) mainConnection);
};
