/**
   Autores de la práctica:
   - Alejandro Rodríguez Arguimbau
   - Sergi Mayol Matos
   - Nicolás Sanz Tuñón
  
   Significado de los mensajes:
  TOC TOC → quiere entrar al comedor
  |▄|     → se ha sentado a comer
  ¡o      → coge palillo izquierdo
  ¡o¡     → coge palillo derecho
  /o\ ÑAM → está comiendo
  ¡o_     → suelta palillo derecho
  _o      → suelta palillo izquierdo
  |_|     → sale del comedor
*/

// Solamente usamos el nucleo app_cpu por simplicidad
// y teniendo en cuenta que algunos esp32 son unicore
// esp32 unicore -> app_cpu = 0
// esp32 2 core  -> app_cpu = 1 (prog_cpu = 0)

#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif
//#define INCLUDE_vTaskSuspend    1    //ja està posat per defecte sino descomentar es el temps de espera                //infinit als semàfors

/*************************** Variables Globales y definiciones **************************************/

#define NUM_OF_PHILOSOPHERS 5                        // Número de filósofos
#define MAX_NUMBER_ALLOWED (NUM_OF_PHILOSOPHERS - 1) // Número máximo de filósofos en el comedor  (uno menos que el total para evitar deadlock)
#define ESPERA 200                                   // tiempo de espera de vTaskDelay

// Settings
enum
{
  TASK_STACK_SIZE = 2048
}; // Bytes in ESP32, words in vanilla FreeRTOS

// Globales
static SemaphoreHandle_t semaforo_bin;   // Esperar a que se lean los parámetros
static SemaphoreHandle_t semaforo_listo; // Notifica cuando termina la tarea principal
static SemaphoreHandle_t palillo[NUM_OF_PHILOSOPHERS];

//*****************************************************************************
// Tareas

// Método para comer
void comer(void *param)
{
  // Espera dividida por la macro para que la espera
  // sea en tiempo real
  vTaskDelay(ESPERA / portTICK_PERIOD_MS);
  int num;
  char buf[50];
  int randomNumber = random(0, ESPERA); // Número random entre 0 y 200
  // Copiar parámetro e incrementar el contador de semáforos
  num = *(int *)param;
  xSemaphoreGive(semaforo_bin);

  // El filósofo i quiere entrar a comer
  sprintf(buf, "Filósofo %i: TOC TOC", num);
  Serial.println(buf);

  // El filósofo i se ha sentado a comer
  sprintf(buf, "Filósofo %i: |▄|", num);
  Serial.println(buf);

  // El filósofo i coge el palillo izquierdo
  xSemaphoreTake(palillo[num], portMAX_DELAY);
  sprintf(buf, "Filósofo %i: ¡o", num);
  Serial.println(buf);

  // Cuando un filósofo ha cogido el palillo de la izquierda,
  // pasa un tiempo aleatorio pensando en sus cosas de entre 0 y ESPERA
  //(el tiempo de espera definido en el código) hasta coger el de su derecha.
  randomNumber = random(0, ESPERA); // Número random entre 0 y 200
  vTaskDelay(randomNumber / portTICK_PERIOD_MS);

  // El filósofo i coge el palillo derecho
  xSemaphoreTake(palillo[(num + 1) % NUM_OF_PHILOSOPHERS], portMAX_DELAY);
  sprintf(buf, "Filósofo %i: ¡o¡", num);
  Serial.println(buf);

  // Antes de decidirse a comer,
  // todos los filósofos pasan un tiempo aleatorio pensando entre 0 y ESPERA.
  randomNumber = random(0, ESPERA); // Número random entre 0 y 200
  vTaskDelay(randomNumber / portTICK_PERIOD_MS);

  // El filósofo i come
  sprintf(buf, "Filósofo %i: ÑAM", num);
  // Los filósofos pasan un tiempo aleatorio (de entre 0 y ESPERA) comiendo.
  randomNumber = random(0, ESPERA); // Número random entre 0 y 200
  vTaskDelay(randomNumber / portTICK_PERIOD_MS);
  Serial.println(buf);

  // El filósofo i deja el palillo derecho
  xSemaphoreGive(palillo[(num + 1) % NUM_OF_PHILOSOPHERS]);
  sprintf(buf, "Filósofo %i: ¡o_", num);
  Serial.println(buf);

  // El filósofo i deja el palillo izquierdo
  xSemaphoreGive(palillo[num]);
  sprintf(buf, "Filósofo %i: _o", num);
  Serial.println(buf);

  // Notificar que ha acabado y eliminarse
  if (xSemaphoreGive(semaforo_listo))
  {
    sprintf(buf, "Filósofo %i: |_|", num);
    Serial.println(buf);
  }
  vTaskDelete(NULL);
}

// Main

void setup()
{

  char tarea[20];
  // Configurar serial
  Serial.begin(9600);

  // Establecemos semilla en un pin analógico para
  // que la secuencia de num aleatorios sea
  // siempre distinta
  randomSeed(analogRead(A0));

  // Esperar un momento para no perder ningún dato
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  Serial.println("------ Cena de filósofos ------");
  Serial.println("@ filósofo 0");
  Serial.println("@ filósofo 1");
  Serial.println("@ filósofo 2");
  Serial.println("@ filósofo 3");
  Serial.println("@ filósofo 4");

  // Crear los objetos del kernel antes de iniciar las tareas
  semaforo_bin = xSemaphoreCreateBinary();
  semaforo_listo = xSemaphoreCreateCounting(MAX_NUMBER_ALLOWED, 0);
  for (int i = 0; i < NUM_OF_PHILOSOPHERS; i++)
  {
    palillo[i] = xSemaphoreCreateMutex();
  }

  while (1)
  {
    // Los filósofos empiezan a comer
    for (int i = 0; i < NUM_OF_PHILOSOPHERS; i++)
    {
      sprintf(tarea, "Philosopher %i", i);
      xTaskCreatePinnedToCore(comer,
                              tarea,
                              TASK_STACK_SIZE,
                              (void *)&i,
                              1,
                              NULL,
                              app_cpu);
      xSemaphoreTake(semaforo_bin, portMAX_DELAY);
    }

    // Esperar hasta que todos los filósfos hayan comido
    for (int i = 0; i < NUM_OF_PHILOSOPHERS; i++)
    {
      xSemaphoreTake(semaforo_listo, portMAX_DELAY);
    }
  }
  // Print para saber que no se ha producido deadlock en todo el programa
  Serial.println("\nNO ha habido deadlock, el programa ha finalizado!");
}

void loop()
{
  // No hacer nada aquí
}
