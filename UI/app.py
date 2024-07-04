from flask import Flask, request, render_template, jsonify
import requests

app = Flask(__name__)

@app.route('/')
def index():
    return render_template('index.html')

@app.route('/submit', methods=['POST'])
def submit():
    value1 = request.form['value1']
    value2 = request.form['value2']
    
    # Sending data to the first URL
    url1 = "http://onem2m.iiit.ac.in:443/~/in-cse/in-name/AE-SR/SR-AQ/SR-AQ-KH95-01/Data"
    payload1 = f'{{\n    "m2m:cin": {{\n        "con": "[1718116449 , 8 , 8 , 532 , 103 , 31974 , 29.53 , 66.58 , {value1} ]"\n        }}\n}}'
    headers = {
        'X-M2M-Origin': 'Mon04_04_22:Mon04_04_22',
        'Content-Type': 'application/json;ty=4'
    }
    response1 = requests.post(url1, headers=headers, data=payload1)
    print(response1.text)

    # Sending data to the second URL
    url2 = "http://onem2m.iiit.ac.in:443/~/in-cse/in-name/AE-AQ/AQ-KH00-00/Data"
    payload2 = f'{{\n    "m2m:cin": {{\n        "con": "[1718116498, 13.00, 17.24, 19.80, 42.17, 29.62, 27.57, 66.13, 76.00, {value2}, 0, 42, 0]"\n        }}\n}}'
    response2 = requests.post(url2, headers=headers, data=payload2)
    print(response2.text)

    # Fetching the latest data from the given URL
    url3 = "http://dev-onem2m.iiit.ac.in:443/~/in-cse/in-name/AE-AQ/AQ-SN00-00/Data/la"
    headers2 = {
        'X-M2M-Origin': 'dev_guest:dev_guest',
        'Content-Type': 'application/json;ty=4'
    }
    response3 = requests.get(url3, headers=headers2)
    latest_data_json = response3.json()
    latest_data_con = latest_data_json["m2m:cin"]["con"]
    print("Latest data:", latest_data_con)
    
    # Extract window and air purifier statuses
    status_values = latest_data_con.strip('[]').split(',')
    print("Status values:", status_values)
    window_status = status_values[0]
    air_purifier_status =  status_values[1]
    print("Window Status:", window_status)
    print("Air Purifier Status:", air_purifier_status)

    return jsonify({
        'value1': value1,
        'value2': value2,
        'window_status': window_status,
        'air_purifier_status': air_purifier_status
    })

if __name__ == '__main__':
    app.run(debug=True)
