import librosa
import soundfile as sf
import numpy as np

# File paths (update these to your file names/paths)
heartbeat_file = '/Users/chanwoo/Documents/GitHub/heartbeat-bpm/rec.wav'
ambient_file = '/Users/chanwoo/Documents/GitHub/heartbeat-bpm/ambient.wav'
output_file = '/Users/chanwoo/Documents/GitHub/heartbeat-bpm/ambient_adjusted.wav'

# 1. Load the heartbeat file and detect its tempo (BPM)
y_heartbeat, sr = librosa.load(heartbeat_file)

# Print audio file information
print(f"Audio duration: {len(y_heartbeat)/sr:.2f} seconds")
print(f"Sample rate: {sr} Hz")
print(f"Max amplitude: {np.max(np.abs(y_heartbeat)):.3f}")

# Try different onset detection methods
onset_env = librosa.onset.onset_strength(y=y_heartbeat, sr=sr)
tempo = librosa.beat.tempo(onset_envelope=onset_env, sr=sr)
tempo_method1 = float(tempo[0])  # Convert from array to float
print("Tempo estimation method 1:", tempo_method1)

# Alternative method with different parameters
onset_env = librosa.onset.onset_strength(y=y_heartbeat, sr=sr, 
                                       hop_length=512, 
                                       aggregate=np.median)
paulxstretch = [librosa.beat.tempo(onset_envelope=onset_env, sr=sr)]
tempo_method2 = float(tempo[0])  # Convert from array to float
print("Tempo estimation method 2:", tempo_method2)

# Use the average of the two successful methods
detected_bpm = (tempo_method1 + tempo_method2) / 2
print("Average detected BPM:", detected_bpm)

# Sanity check the result
if detected_bpm < 30 or detected_bpm > 300:  # Reasonable BPM range
    detected_bpm = 80  # Set a reasonable default
    print("Warning: Could not detect valid BPM. Using default value of 80 BPM")

# 2. Define the base BPM of your ambient track
# (This is the BPM the ambient track was originally produced for.)
base_bpm = 100  # Change this to your ambient track's original BPM

# Calculate the time-stretch factor with an amplification multiplier
amplification = 3  # Increase this to make the stretch more pronounced
base_stretch = detected_bpm / base_bpm
stretch_factor = max(1 + (base_stretch - 1) * amplification, 0.1)  # Ensure minimum stretch factor of 0.1
print("Time-stretch factor:", stretch_factor)

# 4. Load the ambient track
y_ambient, sr_ambient = librosa.load(ambient_file)

# 5. Apply time-stretching to adjust the ambient track's tempo
y_adjusted = librosa.effects.time_stretch(y_ambient, rate=stretch_factor)

# 6. Save the adjusted ambient track
sf.write(output_file, y_adjusted, sr_ambient)
print(f"Adjusted ambient track saved as {output_file}")
