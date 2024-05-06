import requests
import time
import random

url = "http://dev-onem2m.iiit.ac.in:443/~/in-cse/in-name/AE-AQ/AQ-SN00-00/Data"

headers = {
    'X-M2M-Origin': 'Tue_20_12_22:Tue_20_12_22',
    'Content-Type': 'application/json;ty=4'
}

while True:
    # Generate a random value of either "1,0" or "0,1"
    con_value = random.choice(["[1,0]", "[0,1]" ,"[n,0]",])
    
    # Create the payload with the random value
    payload = {
        "m2m:cin": {
            "lbl": [
                "AE-AQ",
                "AQ-SN00-00",
                "V4.0.0",
                "AE-AQ-V4.0.0"
            ],
            "con": con_value
        }
    }

    # Send the POST request
    response = requests.post(url, headers=headers, json=payload)

    # Print the response
    print(response.text)

    # Wait for 10 minutes before sending the next request
    time.sleep(60)  # 10 minutes = 10 * 60 second