import matplotlib.pyplot as plt

loss = [0, 5, 10, 20]

sw_time = [0.049, 7.607, 11.735, 30.763]
gbn_time = [0.032, 9.060, 9.697, 27.844]
sr_time = [0.037, 6.126, 8.420, 16.609]

sw_retrans = [0, 7.33, 11.67, 30.33]
gbn_retrans = [0, 34.33, 37.33, 107.67]
sr_retrans = [0, 7.00, 11.67, 28.33]

sw_goodput = [9198.791, 59.437, 38.252, 15.460]
gbn_goodput = [13946.268, 53.212, 50.600, 16.145]
sr_goodput = [11909.853, 82.573, 54.795, 26.895]

sw_attempts = [109.00, 116.33, 120.67, 139.33]
gbn_attempts = [109.00, 143.33, 146.33, 216.67]
sr_attempts = [109.00, 116.00, 120.67, 137.33]


plt.figure(figsize=(8, 6))

plt.plot(
    loss,
    sw_time,
    marker='o',
    label='Stop-and-Wait'
)

plt.plot(
    loss,
    gbn_time,
    marker='o',
    label='Go-Back-N'
)

plt.plot(
    loss,
    sr_time,
    marker='o',
    label='Selective Repeat'
)

plt.xlabel('Packet Loss (%)')
plt.ylabel('Average Transfer Time (seconds)')
plt.title('Packet Loss vs Transfer Time')

plt.xticks(loss)

plt.grid(True)
plt.legend()

plt.tight_layout()

plt.savefig('../graphs/transfer_time.png')

plt.close()


plt.figure(figsize=(8, 6))

plt.plot(
    loss,
    sw_retrans,
    marker='o',
    label='Stop-and-Wait'
)

plt.plot(
    loss,
    gbn_retrans,
    marker='o',
    label='Go-Back-N'
)

plt.plot(
    loss,
    sr_retrans,
    marker='o',
    label='Selective Repeat'
)

plt.xlabel('Packet Loss (%)')
plt.ylabel('Average Retransmissions')
plt.title('Packet Loss vs Retransmissions')

plt.xticks(loss)

plt.grid(True)
plt.legend()

plt.tight_layout()

plt.savefig('../graphs/retransmissions.png')

plt.close()


plt.figure(figsize=(8, 6))

plt.plot(
    loss,
    sw_goodput,
    marker='o',
    label='Stop-and-Wait'
)

plt.plot(
    loss,
    gbn_goodput,
    marker='o',
    label='Go-Back-N'
)

plt.plot(
    loss,
    sr_goodput,
    marker='o',
    label='Selective Repeat'
)

plt.xlabel('Packet Loss (%)')
plt.ylabel('Average Goodput (kbps)')
plt.title('Packet Loss vs Goodput')

plt.xticks(loss)

plt.grid(True)
plt.legend()

plt.tight_layout()

plt.savefig('../graphs/goodput_overall.png')

plt.close()


loss_focus = [5, 10, 20]

sw_goodput_focus = [
    59.437,
    38.252,
    15.460
]

gbn_goodput_focus = [
    53.212,
    50.600,
    16.145
]

sr_goodput_focus = [
    82.573,
    54.795,
    26.895
]

plt.figure(figsize=(8, 6))

plt.plot(
    loss_focus,
    sw_goodput_focus,
    marker='o',
    label='Stop-and-Wait'
)

plt.plot(
    loss_focus,
    gbn_goodput_focus,
    marker='o',
    label='Go-Back-N'
)

plt.plot(
    loss_focus,
    sr_goodput_focus,
    marker='o',
    label='Selective Repeat'
)

plt.xlabel('Packet Loss (%)')
plt.ylabel('Average Goodput (kbps)')
plt.title('Goodput Comparison Under Packet Loss')

plt.xticks(loss_focus)

plt.grid(True)
plt.legend()

plt.tight_layout()

plt.savefig('../graphs/goodput_loss_only.png')

plt.close()


plt.figure(figsize=(8, 6))

plt.plot(
    loss,
    sw_attempts,
    marker='o',
    label='Stop-and-Wait'
)

plt.plot(
    loss,
    gbn_attempts,
    marker='o',
    label='Go-Back-N'
)

plt.plot(
    loss,
    sr_attempts,
    marker='o',
    label='Selective Repeat'
)

plt.xlabel('Packet Loss (%)')
plt.ylabel('Average Send Attempts')
plt.title('Packet Loss vs Send Attempts')

plt.xticks(loss)

plt.grid(True)
plt.legend()

plt.tight_layout()

plt.savefig('../graphs/send_attempts.png')

plt.close()


print("All comparison graphs generated successfully.")
