import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

plt.style.use('bmh')
plt.rcParams['figure.figsize'] = (16, 6)
plt.rcParams['font.size'] = 12

modes = ['send', 'ssend', 'bsend']
nodes_list = ['1node', '2nodes']

for mode in modes:
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(16, 6))

    for nodes in nodes_list:
        filename = f"{mode}_{nodes}.csv"
        try:
            df = pd.read_csv(filename, skipinitialspace=True)
        except FileNotFoundError:
            print(f"Файл {filename} не найден, пропускаем.")
            continue

        # 1. Расчет альфы и беты по принципу "альфа с первой точки, бетта через последнюю"
        m_first = df['bytes'].iloc[0]
        t_first = df['time_us'].iloc[0]

        m_last = df['bytes'].iloc[-1]
        t_last = df['time_us'].iloc[-1]

        alpha = t_first
        beta = (t_last - alpha) / (m_last - m_first)

        # 2. Теоретическая модель t(m) = alpha + beta * m
        m_theory = np.logspace(0, 6, 100)
        t_theory = alpha + beta * m_theory

        bw_theory = m_theory / t_theory

        label = "1 узел" if nodes == '1node' else "2 узла"

        ax1.plot(df['bytes'], df['time_us'], 'o-', label=f"{label} (эксп.)", markersize=5)
        ax1.plot(m_theory, t_theory, '--', linewidth=2, label=f"{label} (теория: α={alpha:.2f} мкс)")

        bw_exp = df['bytes'] / df['time_us']
        ax2.plot(df['bytes'], bw_exp, 'o-', label=f"{label} (эксп.)", markersize=5)
        ax2.plot(m_theory, bw_theory, '--', linewidth=2, label=f"{label} (теория: 1/β={1 / beta:.2f} МБ/с)")

    for ax in (ax1, ax2):
        ax.set_xscale('log')
        ax.set_xlabel('Размер сообщения, байт')
        ax.grid(True, which="both", ls="--", alpha=0.7)
        ax.legend()

    ax1.set_yscale('log')
    ax1.set_ylabel('Время передачи, мкс')
    ax1.set_title(f'MPI_{mode.capitalize()}: Время передачи сообщений')

    ax2.set_yscale('log')
    ax2.set_ylabel('Пропускная способность, МБ/с')
    ax2.set_title(f'MPI_{mode.capitalize()}: Пропускная способность сети')

    plt.tight_layout()
    plt.savefig(f"{mode}.png", dpi=150)
