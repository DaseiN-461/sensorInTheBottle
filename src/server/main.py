import paho.mqtt.client as mqtt
import subprocess

def topic_exist(msg):
    # Verifica que el topico no exista
    with open("./sensors/list.txt", "r") as file:
        lines = file.readlines()

        for line in lines:          # Se debería leer desde abajo para arriba
                                    # Ya que es mas probable encontrar

            if msg.payload.decode("utf-8") + '\n' == line:
                print("el topico ya se encuentra registrado")
                return True
    print("el topico no se encontraba registrado")
    return False

def topic_in_whitelist(msg):
    # El topico está en la whitelist
    with open("./whitelist.txt", "r") as file:
        lines = file.readlines()

        for line in lines:

            if msg.payload.decode("utf-8") + '\n' == line:
                print("El topico se encuentra en la whitelist")
                return True
        print("el topico no se encontró en la whitelist")
        return False
      

def topic_verify(msg):



    # El topico no existe, procede a registrar el sensor   
    if topic_exist(msg) != True and topic_in_whitelist(msg) == True:        
        
            topic_name = msg.payload.decode("utf-8")

            #Lo añade a la lista de sensores, cada sensor tiene un topico relacionado
            with open("./sensors/list.txt", "a") as file:

                    file.write(topic_name + "\n") 

            # Run topic suscribe
            print("topic name: ["+topic_name+"]")
            subprocess.run(["bash", "sensor_create.sh", topic_name])

            #mala idea, blokea el loop. buscar otra manera de suscribirse
            #subprocess.run(["bash", "./sensors/"+topic_name+"/run.sh"])

            client.subscribe(msg.payload.decode("utf-8"))



#            with open(".sh", "a") as file:  #mode a es para empezar a escribir al final
#                    file.write("mosquitto_sub -h mqtt.eclipseprojects.io -t " + topic_name + " >> " + file_name + "\n")     
#   
#            print("\ntopico:[" + msg.payload.decode("utf-8") + "],  añadido satisfactoriamente.")
#            print("El servidor está a la escucha del sensor mediante el topico mqtt.\n")
    #else:
        # el topico no se encuentra en la whitelist o ya está registrado    


# Callback al recibir un mensaje
def on_message(client, userdata, msg):
    #print(msg.topic + " " + str(msg.payload))
    print("message received")
    
    register_topic = "test/topics"

    if msg.topic == register_topic:
                    print("topic: register")
                    topic_verify(msg)

    else:
                    print("topic: sensor")
                    #registrar en la base de datos
                    with open("./sensors/"+msg.topic+"/measures.txt", "a") as file:
                                    file.write(msg.payload.decode("utf-8")+"\n")





# Crear un cliente MQTT
client = mqtt.Client()
client.on_message = on_message

# Conectar al broker MQTT
client.connect("mqtt.eclipseprojects.io", 1883, 60)

# Suscribirse al tópico de registros
client.subscribe("test/topics")

# Arranca los scripts relacionados a cada topico para subscribirse y registrar los datos
# def func()

# Iniciar el loop de mensajes
client.loop_forever()




