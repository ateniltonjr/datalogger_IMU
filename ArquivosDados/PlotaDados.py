
import numpy as np
import matplotlib.pyplot as plt
import os

# Caminho absoluto do arquivo de dados, relativo ao script
script_dir = os.path.dirname(os.path.abspath(__file__))
data_path = os.path.join(script_dir, 'adc_data3.txt')
data = np.loadtxt(data_path)

x = data[:,0]
y = data[:,1]
plt.plot(x, y, 'blue')
plt.title("Gráfico dos dados coletados")
plt.xlabel("tempo")
plt.ylabel("Amplitude")
plt.grid()
plt.show()