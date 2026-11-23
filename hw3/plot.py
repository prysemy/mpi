import matplotlib.pyplot as plt

p_vdi = [1, 2, 4, 8, 16]
t_vdi = [1.862524, 1.132683, 0.636081, 1.021533, 1.064766]
speedup_vdi = [t_vdi[0] / t for t in t_vdi]

p_calc = [1, 2, 4, 8, 16, 32, 64]
t_calc = [4.153431, 2.086636, 1.044393, 0.521287, 0.263344, 0.148546, 0.094695]
speedup_calc = [t_calc[0] / t for t in t_calc]

ideal_p = [1, 2, 4, 8, 16, 32, 64]
ideal_speedup = [1, 2, 4, 8, 16, 32, 64]

plt.figure(figsize=(10, 6))

plt.plot(p_vdi, speedup_vdi, marker='o', label='Кластер 1 (remote.vdi, старое железо)',
         linewidth=2.5, markersize=8, color='#d62728') # Красный
plt.plot(p_calc, speedup_calc, marker='s', label='Кластер 2 (calc.cod, новое железо)',
         linewidth=2.5, markersize=8, color='#2ca02c') # Зеленый

plt.plot(ideal_p, ideal_speedup, linestyle='--', color='gray',
         label='Идеальное линейное ускорение (S = p)', linewidth=2)

plt.title('Ускорение параллельной игры "Жизнь" (MPI) на двух кластерах',
          fontsize=15, fontweight='bold', pad=15)
plt.xlabel('Количество процессов (p)', fontsize=12)
plt.ylabel('Ускорение S(p) = T(1) / T(p)', fontsize=12)

plt.xticks([1, 2, 4, 8, 16, 32, 64])
plt.yticks(range(0, 18, 2))
plt.grid(True, linestyle=':', alpha=0.7)
plt.legend(fontsize=11, loc='upper left')

plt.savefig('speedup.png', dpi=300, bbox_inches='tight')
plt.show()