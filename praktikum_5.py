from paho.mqtt import client as mqtt_client
import time
import sqlite3
import os
broker = '0.0.0.0'
port = 1883
data = {'rfid':None, 'Bolt':None, 'Nut silver':None, 'Nut black':None, 'Washer silver':None, 'Washer black':None}
# username = 'emqx'
# password = 'public'


def save_product_to_database(Station_ID, ID, Date, Product_type):
    timestamp = time.strftime('%Y-%mj-%d %H:%M:%S')
    query = f"INSERT INTO info_table (Timestamp, Station_ID, ID, Date, Product_type) VALUES (?, ?, ?, ?, ?)"
    cursor1.execute(query, (timestamp, Station_ID, ID, Date, Product_type))
    conn1.commit()
    print("Product data scanned and updated!")


def save_quantity_to_database(Bolt, Nut_Silver, Nut_Hitam, Washer_Silver, Washer_Hitam):
    timestamp = time.strftime('%Y-%m-%d %H:%M:%S')
    query = f"INSERT INTO quantity_table (Timestamp, Bolt, 'Nut M12 Silver', 'Nut M12 Hitam', 'Washer Silver', 'Washer Hitam') VALUES (?, ?, ?, ?, ?, ?)"
    cursor2.execute(query, (timestamp, Bolt, Nut_Silver, Nut_Hitam, Washer_Silver, Washer_Hitam))
    conn2.commit()
    print("Quantity data scanned and updated in 'Quantity data' database!")
    
def connect_mqtt() -> mqtt_client:
    def on_connect(client, userdata, flags, rc):
        if rc == 0:
            print("Connected to MQTT Broker!")
        else:
            print("Failed to connect, return code %d\n", rc)
    client = mqtt_client.Client('Python1')
    client.on_connect = on_connect
    client.connect(broker, port)
    return client


def subscribe(client: mqtt_client):
    def on_message(client, userdata, msg):
        print(f"Received `{msg.payload.decode()}` from `{msg.topic}` topic")
        if msg.topic == 'rfid':
            data['rfid'] = msg.payload.decode()
        if msg.topic == 'Bolt':
            data['Bolt'] = msg.payload.decode()
        if msg.topic == 'Nut silver':
            data['Nut silver'] = msg.payload.decode()
        if msg.topic == 'Nut black':
            data['Nut black'] = msg.payload.decode()
        if msg.topic == 'Washer silver':
            data['Washer silver'] = msg.payload.decode()
        if msg.topic == 'Washer black':
            data['Washer black'] = msg.payload.decode()
        #jika semua data masuk maka akan dimasukan ke database
        if data['rfid'] and data['Bolt']  and data['Nut silver']  and data['Nut black']  and data['Washer silver']  and data['Washer black'] :
            print('semua data masuk')
            Station_ID = data['rfid'][12]
            ID = data['rfid'][19:26]
            Date = data['rfid'][33:39]
            Product_type = data['rfid'][-1]
            
            save_product_to_database(Station_ID, ID, Date, Product_type)
            
            save_quantity_to_database(data['Bolt'], data['Nut silver'], data['Nut black'], data['Washer silver'], data['Washer black'])
            
            #kosongkan kembali variable dictionary data
            for topic in data:
                data[topic] = None
                
                
    client.subscribe('rfid')
    client.subscribe('Bolt')
    client.subscribe('Nut silver')
    client.subscribe('Nut black')
    client.subscribe('Washer silver')
    client.subscribe('Washer black')
    client.on_message = on_message



    # Define database file paths
INFO_DB_FILE = 'info_data.db'
QUANTITY_DB_FILE = 'quantity_data.db'

# Check if the database files exist
if not os.path.exists(INFO_DB_FILE):
    # Create the "Info data" database and its tables
    conn1 = sqlite3.connect(INFO_DB_FILE)
    cursor1 = conn1.cursor()
    cursor1.execute('''CREATE TABLE info_table (
                        Timestamp TEXT,
                        Station_ID TEXT,
                        ID TEXT,
                        Date TEXT,
                        Product_type TEXT
                        )''')
    conn1.commit()
    conn1.close()

if not os.path.exists(QUANTITY_DB_FILE):
    # Create the "Quantity data" database and its tables
    conn2 = sqlite3.connect(QUANTITY_DB_FILE)
    cursor2 = conn2.cursor()
    cursor2.execute('''CREATE TABLE quantity_table (
                        Timestamp TEXT,
                        Bolt INTEGER,
                        'Nut M12 Silver' INTEGER,
                        'Nut M12 Hitam' INTEGER,
                        'Washer Silver' INTEGER,
                        'Washer Hitam' INTEGER
                        )''')
    conn2.commit()
    conn2.close()

# Establish connections to the databases
conn1 = sqlite3.connect(INFO_DB_FILE)
conn2 = sqlite3.connect(QUANTITY_DB_FILE)

# Create cursor objects for each connection
cursor1 = conn1.cursor()
cursor2 = conn2.cursor()

#mqtt
client = connect_mqtt()
subscribe(client)
client.loop_forever()

