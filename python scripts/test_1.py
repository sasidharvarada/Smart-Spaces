from flask import Flask, jsonify
import requests

app = Flask(__name__)

@app.route('/')
def get_ack_airpurifier():
    url = "http://dev-onem2m.iiit.ac.in:443/~/in-cse/in-name/AE-AQ/AQ-SN00-00/ACK-AIRPURIFIER/la"
    headers = {
        'X-M2M-Origin': 'Tue_20_12_22:Tue_20_12_22',
        'Content-Type': 'application/json;ty=4'
    }

    response = requests.get(url, headers=headers)

    try:
        ack_airpurifier = response.json()["m2m:cin"]["con"]
        ack_airpurifier_ct = response.json()["m2m:cin"]["ct"]

        ack = {ack_airpurifier_ct: ack_airpurifier}
        return jsonify(ack)
    except KeyError:
        return jsonify({"error": "Failed to retrieve data from the server"}), 500

if __name__ == '__main__':
    app.run(debug=True)