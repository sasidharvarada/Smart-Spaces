import time
import random
import requests

# WiFi Configuration (not required for Python script)
# Define constants

CSE_IP = "dev-onem2m.iiit.ac.in"
CSE_PORT = 443
HTTPS = False
OM2M_ORGIN = "Tue_20_12_22:Tue_20_12_22"
OM2M_MN = "/~/in-cse/in-name/"
OM2M_AE = "AE-AQ/AQ-SN00-00"
OM2M_DATA_CONT = "AQ-SN00-00/Data"

# List of possible data values
data_options = [
    ["CLOSE", "ON"],
    ["OPEN", "OFF"],
    ["NC", "OFF"],
    ["CLOSE", "OFF"]
]

# Function to send data to server
def send_data(data):
    server = "http"
    if HTTPS:
        server += "s"
    server += f"://{CSE_IP}:{CSE_PORT}{OM2M_MN}"

    url = server + OM2M_AE + "/" + OM2M_DATA_CONT
    headers = {
        "X-M2M-Origin": OM2M_ORGIN,
        "Content-Type": "application/json;ty=4",
        "Connection": "close"  # Debugging header
    }

    req_data = {
        "m2m:cin": {
            "con": data,
            "cnf": "text"
        }
    }

    print("Server URL: " + url)
    print("Request Data: " + str(req_data))

    response = requests.post(url, json=req_data, headers=headers)
    print(response.status_code)
    response.close()

# Main loop
while True:
    data = random.choice(data_options)
    print("Sending Data: ", data)

    send_data(data)
    time.sleep(180)  # Sleep for 3 minutes (180 seconds)
