#pragma once
#include <iostream>
#include "IMqttListener.h"

class ConsoleLogger : public IMqttListener
{
public:
    void onMessageReceived(const std::string& topic, const std::string& payload) override
   {
       std::cout << "\n[MQTT EVENT] Comando recibido en el canal '" << topic << "': " << payload << std::endl;
  
       // Simulación de control de dispositivo:
       if (topic == "home/actuators/light")
       {
           if (payload == "ON") 
           {
               std::cout << ">>> ACCION: Encendiendo la luz principal..." << std::endl;
           } else if (payload == "OFF") 
           {
               std::cout << ">>> ACCION: Apagando la luz principal..." << std::endl;
           }
       }
   }
};
