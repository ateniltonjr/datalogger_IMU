
import numpy as np
import matplotlib.pyplot as plt
import os

# Caminho absoluto do diretório do script
script_dir = os.path.dirname(os.path.abspath(__file__))

# Lista de arquivos e configurações de plot
arquivos = [
    ('adc_data3.txt', 'green', 'Dados 1'),
    ('adc_data4.txt', 'yellow', 'Dados 3'),
]

for nome_arquivo, cor, legenda in arquivos:
    data_path = os.path.join(script_dir, nome_arquivo)
    data = np.loadtxt(data_path)
    x = data[:, 0]
    y = data[:, 1]
    plt.plot(x, y, color=cor, label=legenda)

plt.title("Gráfico dos dados coletados")
plt.xlabel("tempo")
plt.ylabel("Amplitude")
plt.grid()
plt.legend()
plt.show()