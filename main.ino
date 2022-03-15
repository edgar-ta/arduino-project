/**
 * © Copyright 2022 Edgar Trejo Avila
 * 
 * This program is free software: you can redistribute it and/or modify it under 
 * the terms of the GNU General Public License as published by the Free Software Foundation, 
 * either version 3 of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; 
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along with this program. 
 * If not, see <https://www.gnu.org/licenses/>. 
 * 
 * */

/** 
 * Lectura que indica que el sensor no está detectando nada 
 * */
#define NULL_READING 0

/**
 * El nombre es self-explanatory
 * */
#define PLACEHOLDER_NUMBER 1

/**
 * Velocidad máxima (inclusiva) que un objeto puede tener
 * sin ser considerado peligroso; en m/s
 * */
#define EMERGENCY_SPEED 2

/**
 * Distancia mínima (inclusiva) que un objeto puede tener
 * sin ser considerado peligroso (en m); un objeto
 * más cercano que esto es considerado peligroso
 * */
#define MODERATE_DISTANCE 0.5

/**
 * Velocidad de una onda infrarroja (o lo que sea que vamos a utilizar
 * para medir); en m/s
 * */
#define WAVE_SPEED 300

class MotionConnection;

/**
 * Convierte una lectura a metros.   
 * 
 * Para pasar de microsegundos a metros, primero
 * se dividen dichos microsegundos por 10**6 para convertirlos a segundos
 * (recordemos que un microsegundo es un millonésimo
 * de segundo), luego se multiplican esos segundos por la wave speed
 * y se obtiene la distancia que la onda recorrió de ida y vuelta,
 * solo requerimos la distancia de ida (para saber qué tan lejos
 * del sensor está el objeto) y, ya que la distancia de ida y vuelta
 * son la misma, se divide entre dos.
 * 
 * Se acomoda la ecuación para dejarla estéticamente más correcta
 * y ya está.
 * 
 * @param reading Lectura del sensor (en m/μs)
 * @returns Distancia (en metros) que corresponde a la lectura dada
 * */
double parseReading(unsigned long reading)
{
    return reading * WAVE_SPEED / (2 * pow(10, 6));
};

void onEmergency(MotionConnection* connection){
    //
};

void onModerate(MotionConnection* connection){
    // 
    unsigned long long something = 32.402;
};

void onSafe(MotionConnection* connection){
    //
};

/**
 * Direcciones en las que un objeto se mueve respecto
 * al sensor.
 * */
typedef enum
{
    /**
     * Alemán; que se está alejando
     * */
    HIN,

    /**
     * Alemán; que se está acercando
     * */
    HER,

    /**
     * Dirección desconocida
     * */
    UNKNOWN
} Direction;

/**
 * Clase que monitorea datos de un sensor de distancia, calcula y actúa
 * según es necesario 
 * */
class MotionConnection
{
public:
    /** 
     * El pin (número de pin) a utilizar para leer
     * datos del sensor de distancia 
     * */
    short input;

    /**
     * El pin (número de pin) a utilizar para controlar
     * el sensor (hacerlo prender y apagar)
     * */
    short output;

    /**
     * Flag; indica si la conexión está recibiendo y calculando datos o no.
     * Cuando la conexión está inactiva, tiene valores por defecto
     * */
    boolean isActive;

    /**
     * La distancia anterior capturada por la conexión (m)
     * */
    double distance;

    /**
     * La velocidad anterior capturada por la conexión (m/s)
     * */
    double speed;

    /**
     * El tiempo en que los datos anteriores fueron capturados por
     * la conexión (ms)
     * */
    unsigned long time;

    /**
     * Dirección anterior capturada por la conexión
     * */
    Direction direction;

protected:
    void init(short input, short output, bool isActive, double distance, double speed, unsigned long time, Direction direction)
    {
        this->input = input;
        this->output = output;
        this->isActive = isActive;
        this->distance = distance;
        this->speed = speed;
        this->time = time;
        this->direction = direction;
    }

public:
    /**
     * Constructor
     * 
     * @param input Pin a utilizar para recibir datos del sensor
     * @param output Pin a utilizar para activar el sensor
     * */
    MotionConnection(short input, short output)
    {
        pinMode(input, INPUT);
        pinMode(output, OUTPUT);
        init(input, output, false, PLACEHOLDER_NUMBER, PLACEHOLDER_NUMBER, PLACEHOLDER_NUMBER, UNKNOWN);
    };

    /**
     * @brief Inicia una conexión
     *
     * Guarda el tiempo actual (dado por la built-in millis),
     * la distancia dada, y cambia el estado de la conexión a activo
     *
     * @param distance La distancia
     * */
    void startConnection(double distance)
    {
        this->distance = distance;
        this->time = millis();
        this->isActive = true;
    }

    /**
     * Calcula los nuevos datos de la conexión
     * */
    void continueConnection(double distance) {
        unsigned long time = millis();
        double distanceDelta = this->distance - distance;
        unsigned long timeDelta = this->time - time;

        // the speed is measured in m/s, but the time delta was given in ms
        this->speed = 1000 * distanceDelta / timeDelta;
        this->direction = distanceDelta < 0 ? HIN : HER;
        this->time = time;
        this->distance = distance;
    }

    /**
     * Termina la conexión
     *
     * ---
     * Si la conexión no está activa; solo regresa.
     * Si sí lo está, se inicializa con los valores por defecto
     * (como en el constructor), pero esta vez pasa sus propios
     * pines en vez de solicitarlos
     * */
    void endConnection()
    {
        if (this->isActive) 
        this->init(this->input, this->output, false, PLACEHOLDER_NUMBER, PLACEHOLDER_NUMBER, PLACEHOLDER_NUMBER, UNKNOWN);
    };

    void act()
    {
        if (this->direction == HER && this->speed >= EMERGENCY_SPEED) return onEmergency(this);
        if (this->direction == HER || this->distance <= MODERATE_DISTANCE) return onModerate(this);
        onSafe(this);
    };

    void listen()
    {
        double distance = this->getDistance();
        if (distance == NULL_READING)
        {
            this->endConnection();
            return;
        }
        if (this->isActive) this->startConnection(distance);
        else this->continueConnection(distance);
        this->act();
    };

    double getDistance()
    {
        digitalWrite(this->output, HIGH);
        // the timeout is 1 second by default; could change
        // depending on the maximum distance we intend to measure. also,
        // the documentation states that i should call the
        // method a few microseconds before the actual pulse begins
        // so i compensed it with the addition
        unsigned long reading = pulseIn(this->input, HIGH);
        digitalWrite(this->output, LOW);
        if (reading == NULL_READING) return NULL_READING;
        return parseReading(reading + 24);
    };
};

MotionConnection mainConnection(5, 1);

void setup() {};

void loop()
{
    mainConnection.listen();
};
