//Projet RTOS - Raphaël GANDUS - ING2 Instrumentation - 2022

//Importation des librairies
#include <Arduino_FreeRTOS.h>
#include "queue.h"
#include "semphr.h"
#include "task.h"

//Attribution des ports
#define BUTTON1 3
#define BUTTON2 4
#define ANALOG A0

// Priorités auxquelles les tâches sont créées
#define PRIORITY_TASK_1  (tskIDLE_PRIORITY+1) 
#define PRIORITY_TASK_2  (tskIDLE_PRIORITY+1) 
#define PRIORITY_TASK_3  (tskIDLE_PRIORITY+2)
#define PRIORITY_TASK_4  (tskIDLE_PRIORITY+2) 
#define PRIORITY_TASK_5  (tskIDLE_PRIORITY+3)

//Déclaration des queues
QueueHandle_t SendToTask3_1;
QueueHandle_t SendToTask3_2;
QueueHandle_t SendToTask4;
QueueHandle_t SendToTask5;

//Déclaration du sémaphore
SemaphoreHandle_t sem; 

//Déclaration de la structure pour les tâches 3, 4 et 5
typedef struct valeurCapteurs 
{
    int analogique;
    int numerique;
    double tempsEnMillisecondes;
}valeurCapteurs;

unsigned long startTime;  


void setup()
{
  //Initialisation
  Serial.begin(9600);       //Port communication série

  //Etat des ports
  pinMode(BUTTON1, INPUT);  
  pinMode(BUTTON2, INPUT);
  pinMode(ANALOG, INPUT);

  //Initialisation du sémaphore
  sem = xSemaphoreCreateBinary();

  //Définition des queues
  SendToTask3_1 = xQueueCreate(1, sizeof(int)); //File d'attente pour envoyer des données de la tâche 1 à la tâche 3
  SendToTask3_2 = xQueueCreate(1, sizeof(int)); //File d'attente pour envoyer des données de la tâche 2 à la tâche 3
  SendToTask4 = xQueueCreate(1, sizeof(valeurCapteurs)); //File d'attente pour envoyer des données de la tâche 3 à la tâche 4
  SendToTask5 = xQueueCreate(1, sizeof(valeurCapteurs)); //File d'attente pour envoyer des données de la tâche 4 à la tâche 5
  
  //Création des tâches
  xTaskCreate(tache1, "tache1", 1000, NULL, PRIORITY_TASK_1, NULL);   //Tâche 1
  xTaskCreate(tache2, "tache2", 1000, NULL, PRIORITY_TASK_2, NULL);   //Tâche 2
  xTaskCreate(tache3, "tache3", 1000, NULL, PRIORITY_TASK_3, NULL);   //Tâche 3
  xTaskCreate(tache4, "tache4", 1000, NULL, PRIORITY_TASK_4, NULL);   //Tâche 4
  xTaskCreate(tache5, "tache5", 1000, NULL, PRIORITY_TASK_5, NULL);   //Tâche 5

  //Création de l'ordonnanceur
  vTaskStartScheduler(); 
}    

void tache1(void* pvParameters) //Fonction pour la tâche 1 : lit la valeur du potentiometre et l'envoie à la tâche 3
{
    while (1)
  {
    int analogValue = analogRead(ANALOG);
    int outputValue = analogValue;

    xQueueSend(SendToTask3_1, &outputValue, portMAX_DELAY);
  }
}

void tache2(void* pvParameters) //Fonction pour la tâche 2 : calcule la résultante de l'addition des deux valeurs des deux entrées numérique 3 et 4 et l'envoie à la tâche 3
{
  while(1)
  {
    int button1State = digitalRead(BUTTON1);
    int button2State = digitalRead(BUTTON2);
    int digitalValue = button1State + button2State;

    xQueueSend(SendToTask3_2, &digitalValue, portMAX_DELAY);
  }
}

void tache3(void* pvParameters) //Fonction pour la tâche 3 : réception des données des tâches 1 et 2, stockage dans une structure et envoi de la structure à la tâche 4
{
  while(1)
  {
    double startTime = millis();

    valeurCapteurs structValeurs;
    int receivedAnalogValue;
    int receivedDigitalValue;

    xQueueReceive(SendToTask3_1, &receivedAnalogValue, portMAX_DELAY);
    xQueueReceive(SendToTask3_2, &receivedDigitalValue, portMAX_DELAY);

    structValeurs.analogique = receivedAnalogValue;
    structValeurs.numerique = receivedDigitalValue;
    structValeurs.tempsEnMillisecondes = startTime;

    xQueueSend(SendToTask4, &structValeurs, portMAX_DELAY);
  }
}

void tache4(void* pvParameters)   //Fonction pour la tâche 4 : reçoit la structure de la tâche 3, affiche la structure et la renvoie à la tâche 5
{
  while(1)
  {
    valeurCapteurs structValeurs;
    xQueueReceive(SendToTask4, &structValeurs, portMAX_DELAY);

    //Affichage sur le port série
    Serial.println("**********TACHE 4**********");
    Serial.print("Analogique : ");
    Serial.println(structValeurs.analogique);
    Serial.print("Numerique : ");
    Serial.println(structValeurs.numerique);
    Serial.print("Temps écoulé (en ms) : ");
    Serial.println(structValeurs.tempsEnMillisecondes);

    xQueueSend(SendToTask5, &structValeurs, portMAX_DELAY);

    xSemaphoreGive(sem);  // Ouverture du sémaphore pour débloquer la tâche 5

    //Temporisation
    vTaskDelay(50);
  }
}

void tache5(void* pvParameters)   //Fonction pour la tâche 5 : reçoit la structure de la tâche 4, transforme la valeur du temps en minutes, et affiche la nouvelle structure
{
  while(1)
  {
    xSemaphoreTake(sem,portMAX_DELAY); // Attente sur sémaphore (ouvert par la tâche 4)

    valeurCapteurs structValeurs;

    xQueueReceive(SendToTask5, &structValeurs, portMAX_DELAY);

    //Conversion du temps (en ms) en minutes
    structValeurs.tempsEnMillisecondes = structValeurs.tempsEnMillisecondes*(0.001/60);

    
    //Affichage sur le port série
    Serial.println("**********TACHE 5**********");
    Serial.print("Analogique : ");
    Serial.println(structValeurs.analogique);
    Serial.print("Numerique : ");
    Serial.println(structValeurs.numerique);
    Serial.print("Temps écoulé (en minutes) : ");
    Serial.println(structValeurs.tempsEnMillisecondes);
    
    //Temporisation
    vTaskDelay(50);
  }
}


void loop()
{
  //Rien dans cette fonction
}