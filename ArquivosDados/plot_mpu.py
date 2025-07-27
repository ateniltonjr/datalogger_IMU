import numpy as np
import matplotlib.pyplot as plt
import os

# Caminho absoluto do arquivo de dados, relativo ao script
script_dir = os.path.dirname(os.path.abspath(__file__))
data_path = os.path.join(script_dir, 'dadosMPU2.csv')
data = np.loadtxt(data_path, delimiter=',')

amostra = data[:, 0]
accel_x = data[:, 1]
accel_y = data[:, 2]
accel_z = data[:, 3]
giro_x = data[:, 4]
giro_y = data[:, 5]
giro_z = data[:, 6]

fig, axs = plt.subplots(3, 2, figsize=(12, 8))

axs[0, 0].plot(amostra, accel_x, 'b')
axs[0, 0].set_title('Aceleração X')
axs[0, 0].set_xlabel('Número da amostra')
axs[0, 0].set_ylabel('Accel X')
axs[0, 0].grid()

axs[1, 0].plot(amostra, accel_y, 'g')
axs[1, 0].set_title('Aceleração Y')
axs[1, 0].set_xlabel('Número da amostra')
axs[1, 0].set_ylabel('Accel Y')
axs[1, 0].grid()

axs[2, 0].plot(amostra, accel_z, 'r')
axs[2, 0].set_title('Aceleração Z')
axs[2, 0].set_xlabel('Número da amostra')
axs[2, 0].set_ylabel('Aceleração Z')
axs[2, 0].grid()

axs[0, 1].plot(amostra, giro_x, 'c')
axs[0, 1].set_title('Giro X')
axs[0, 1].set_xlabel('Número da amostra')
axs[0, 1].set_ylabel('Giro X')
axs[0, 1].grid()

axs[1, 1].plot(amostra, giro_y, 'm')
axs[1, 1].set_title('Giro Y')
axs[1, 1].set_xlabel('Número da amostra')
axs[1, 1].set_ylabel('Giro Y')
axs[1, 1].grid()

axs[2, 1].plot(amostra, giro_z, 'y')
axs[2, 1].set_title('Giro Z')
axs[2, 1].set_xlabel('Número da amostra')
axs[2, 1].set_ylabel('Giro Z')
axs[2, 1].grid()

plt.tight_layout()
plt.show()