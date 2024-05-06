import pandas as pd
import matplotlib.pyplot as plt
from matplotlib.lines import Line2D

# Define the file path
file_path = r'd:\OneDrive - International Institute of Information Technology\Sasidhar\SCRC\Smart Spaces\06-01-2024\graph_log.xlsx'

try:
    # Load the data into a pandas DataFrame
    data = pd.read_excel(file_path)

    # Get indices where 'State' changes
    state_change_indices = data[data['State'] != data['State'].shift()].index

    # Plotting the indoor and outdoor AQI values
    fig, ax = plt.subplots(figsize=(12, 6))

    # Plot indoor and outdoor AQI
    indoor_plot, = ax.plot(data['current_time'], data['indoor aqi'], label='Indoor AQI', color='blue')
    outdoor_plot, = ax.plot(data['current_time'], data['outdoor aqi'], label='Outdoor AQI', color='green')

    # Plot vertical lines at state change points and add text annotations between lines
    for i in range(len(state_change_indices) - 1):
        start_index = state_change_indices[i]
        end_index = state_change_indices[i + 1]
        mid_index = (start_index + end_index) // 2 
        state_name = data['State'][mid_index].strip(' -')
        
        # Adjust vertical position to be in the range of AQI 550 to 750
        vertical_position = 450
        ax.text(data['current_time'][mid_index], vertical_position, f'{state_name} ', rotation=0,
                verticalalignment='bottom', horizontalalignment='center',
                fontsize=8, fontweight='bold')

        # Plot double yellow lines at state change points

    # Set y-axis limit to not exceed 2000
    ax.set_ylim(0, 2000)

    # Plot vertical lines at state change points
    for i, index in enumerate(state_change_indices):
        color = 'red' if i != 0 and i != len(state_change_indices) - 1 else 'red'
        ax.axvline(x=data['current_time'][index], color=color, linestyle='--', alpha=0.5)

    # Define custom legend entries
    custom_legend = [
        Line2D([0], [0], color='red', linestyle='--', label='State Change'),
        Line2D([0], [0], color='blue', label='Indoor AQI'),
        Line2D([0], [0], color='green', label='Outdoor AQI'),
        Line2D([0], [0], color='white', label='STATE 1: NO CHANGE, OFF'),
        Line2D([0], [0], color='white', label='STATE 2: CLOSE, OFF'),
        Line2D([0], [0], color='white', label='STATE 3: OPEN, OFF'),
        Line2D([0], [0], color='white', label='STATE 4: CLOSE, ON')
    ]

    # Add custom legend entries and show legend
    ax.legend(handles=custom_legend, loc='upper left')
    ax.set_ylabel('AQI')
    ax.set_xlabel('Timestamp (06-01-2024)')

    # Format x-axis ticks as (hh:mm:ss)
    ax.xaxis.set_major_formatter(plt.matplotlib.dates.DateFormatter('%H:%M:%S'))

    plt.title('Indoor and Outdoor AQI with State Change Points')
    plt.tight_layout()
    plt.xticks(rotation=45)  # Rotate x-axis labels for better readability
    plt.show()

except FileNotFoundError:
    print("File not found. Please provide a valid file path.")
