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
 * Valor de una duración que no existe (cuando no hay objeto cercano)
 * */
#define NULL_DURATION 0

/**
 * Valor de una distancia que no existe (cuando no hay objeto cercano)
 * */
#define NULL_DISTANCE -1

/**
 * El nombre es self-explanatory
 * */
#define PLACEHOLDER_NUMBER 0

/**
 * Velocidad máxima (exclusiva) que un objeto puede tener
 * sin ser considerado peligroso; en m/s.
 * 
 * Todas las velocidades mayores a esta serán consideradas
 * peligrosas.
 * */
#define SAFE_SPEED 2

/**
 * Distancia mínima (exclusiva) que un objeto puede tener
 * sin ser considerado peligroso (en m).
 * 
 * Todas las distancias menores a esta serán consideradas
 * peligrosas.
 * */
#define SAFE_DISTANCE 0.5

/**
 * Velocidad de una onda de sonido; en m/s
 * */
#define SOUND_SPEED 333

/**
 * Convierte una duración a metros.   
 * 
 * ---
 * Para pasar de microsegundos a metros, primero
 * se dividen dichos microsegundos por 10**6 para convertirlos a segundos
 * (recordemos que un microsegundo es un millonésimo
 * de segundo), luego se multiplican esos segundos por la velocidad del sonido
 * y se obtiene la distancia que la onda recorrió de ida y vuelta,
 * solo requerimos la distancia de ida (para saber qué tan lejos
 * del sensor está el objeto) y, ya que la distancia de ida y vuelta
 * son la misma, se divide entre dos.
 * 
 * Se acomoda la ecuación para dejarla estéticamente más correcta
 * y ya está.
 * 
 * @param duration Duración medida por el sensor (en μs)
 * @returns Distancia (en m) que corresponde a la duración dada
 * */
double getDistanceFromDuration(unsigned long duration)
{
    return duration * SOUND_SPEED / (2 * pow(10, 6));
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
     * Placeholder
     * */
    PLACEHOLDER
} Direction;

/**
 * Clase que monitorea datos de un sensor de un sensor ultrasónico, calcula y actúa
 * según es necesario 
 * */
class MotionConnection
{
public:
    /** 
     * El pin (número de pin) a utilizar para encender
     * el sensor
     * */
    short trigger;

    /**
     * El pin (número de pin) a utilizar para obtener
     * datos del sensor
     * */
    short listener;

    /**
     * Flag; indica si la conexión está recibiendo y calculando datos o no.
     * 
     * Cuando la conexión está inactiva, tiene valores por defecto 
     * (algunos de los cuales son placeholders)
     * */
    boolean active;

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
    /**
     * Inicializa valores con placeholders
     * 
     * ---
     * Los valores a inicializar son la velocidad, 
     * la distancia, el tiempo y la dirección.
     * La dirección se inicializa con `Direction::PLACEHOLDER`
     * y todos los demás con `PLACEHOLDER_NUMBER`
     * */
    void setPlaceholders() {
        this->speed = PLACEHOLDER_NUMBER;
        this->distance = PLACEHOLDER_NUMBER;
        this->time = PLACEHOLDER_NUMBER;
        this->direction = Direction::PLACEHOLDER;
    }

public:
    /**
     * Constructor
     * 
     * @param input Pin a utilizar para recibir datos del sensor
     * @param output Pin a utilizar para activar el sensor
     * */
    MotionConnection(short listener, short trigger)
    {
        pinMode(listener, INPUT);
        pinMode(trigger, OUTPUT);

        this->active = false;
        this->listener = listener;
        this->trigger = trigger;
        this->setPlaceholders();
    };

    /**
     * Inicia una conexión
     * 
     * ---
     * Guarda el tiempo actual (dado por la built-in millis),
     * la distancia dada, y cambia el estado de la conexión a activo
     *
     * @param distance La distancia con la cual empezar la conexión
     * */
    void startConnection(double distance)
    {
        this->distance = distance;
        this->time = millis();
        this->active = true;
    }

    /**
     * Calcula los nuevos datos de la conexión
     * 
     * @param distance La distancia a utilizar para continuar
     * la conexión
     * */
    void continueConnection(double distance) {
        unsigned long time = millis();
        double distanceDelta = distance - this->distance;
        unsigned long timeDelta = time - this->time;

        // the speed is measured in m/s, but the time delta was given in ms
        this->speed = 1000 * distanceDelta / timeDelta;
        this->direction = distanceDelta > 0 ? HIN : HER;
        this->time = time;
        this->distance = distance;
    }

    /**
     * Termina la conexión
     *
     * ---
     * Si la conexión está activa, pone el estado
     * a inactivo e inicializa con placeholders
     * */
    void endConnection()
    {
        if (this->active) {
            this->active = false;
            this->setPlaceholders();
        }
    };

    /**
     * Actúa dependiendo de los datos recolectados por la conexión.
     * 
     * ---
     * Si la dirección es `HER` (acercándose) y la velocidad es mayor
     * a la `SAFE_SPEED`, se llama a `onHighRisk`.
     * Si la distancia no es nula y es menor a la `SAFE_DISTANCE`, se llama
     * a `onModerateRisk`.
     * Si ninguna de las opciones anteriores ocurrió, se llama a 
     * `onLowRisk`
     * **/
    void act()
    {
        if (this->direction == HER && this->speed >= SAFE_SPEED) this->onHighRisk();
        else if (this->distance != NULL_DISTANCE && this->distance < SAFE_DISTANCE) this->onModerateRisk();
        else this->onLowRisk();
    };

    /**
     * Escucha (se mantiene al tanto) de la velocidad y distancia
     * del objeto más cercano
     * 
     * ---
     * Obtiene la distancia y, si es nula, manda a terminar la conexión,
     * si no es nula, checa si la conexión está activa o no y se manda a llamar a
     * `continueConnection` o `startConnection` (respectivamente),
     * finalmente, se llama a `act`
     * */
    void listen()
    {
        double distance = this->getDistance();
        if (distance == NULL_DISTANCE)
        {
            this->endConnection();
            return;
        }
        if (this->active) this->continueConnection(distance);
        else this->startConnection(distance);
        
        this->act();
    };

    /**
     * Obtiene la distancia (respecto al sensor) a la que está el objeto
     * más cercano
     * 
     * ---
     * Véase el link: 
     * https://create.arduino.cc/projecthub/abdularbi17/ultrasonic-sensor-hc-sr04-with-arduino-tutorial-327ff6
     * */
    double getDistance()
    {
        digitalWrite(this->trigger, HIGH);
        delayMicroseconds(10);
        digitalWrite(this->trigger, LOW);
        unsigned long duration = pulseIn(this->listener, HIGH);
        if (duration == NULL_DURATION) return NULL_DISTANCE;
        return getDistanceFromDuration(duration);
    };

    /**
     * Función a ejecutar cuando el riesgo es alto
     * */
    void onHighRisk() {
        Serial.println("EL_RIESGO_ES_ALTO");
        this->printStatus();
    };

    /**
     * Función a ejecutar cuando el riesgo es moderado
     * */
    void onModerateRisk(){
        Serial.println("ElRiesgoEsModerado");
        this->printStatus();
    };

    /**
     * Función a ejecutar cuando el riesgo es bajo
     * */
    void onLowRisk(){
        Serial.println("el-riesgo-es-bajo");
        this->printStatus();
    };

    /**
     * Función auxiliar (no será implementada en la versión final)
     * que imprime datos de la conexión en pantalla
     * */
    void printStatus() {
        Serial.print("Distancia: ");
        Serial.print(this->distance, 6);
        Serial.println(" m");
        Serial.print("Velocidad: ");
        Serial.print(this->speed, 6);
        Serial.println(" m/s");
        Serial.print("Dirección");
        Serial.print(this->direction == Direction::HER? "HER": "HIN");
        Serial.println("---");
    }
};

/**
 * Conexión principal (hasta ahora la única)
 * 
 * ---
 * Los pines a utilizar se decidieron por conveniencia al ensamblar
 * */
MotionConnection mainConnection(8, 7);

void setup() {
    Serial.begin(9600);
};

void loop()
{
    mainConnection.listen();
};
