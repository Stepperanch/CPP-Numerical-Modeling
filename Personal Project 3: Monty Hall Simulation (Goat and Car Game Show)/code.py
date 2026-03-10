import random
import matplotlib.pyplot as plt

def monty_hall_sim(num_trials=10000):
    stay_wins = 0
    switch_wins = 0

    stay_rates = []
    switch_rates = []

    for i in range(1, num_trials + 1):
        # 0, 1, 2 represent the three doors
        car_door = random.randint(0, 2)
        initial_choice = random.randint(0, 2)

        # Monty opens a door that is not the car door and not the chosen door
        possible_reveal_doors = [d for d in range(3) if d != car_door and d != initial_choice]
        revealed_door = random.choice(possible_reveal_doors)

        # Strategy: Stay
        if initial_choice == car_door:
            stay_wins += 1

        # Strategy: Switch
        # The switch choice is the door that is not the initial choice and not the revealed door
        switch_choice = [d for d in range(3) if d != initial_choice and d != revealed_door][0]
        if switch_choice == car_door:
            switch_wins += 1

        stay_rates.append(stay_wins / i)
        switch_rates.append(switch_wins / i)

    return stay_rates, switch_rates

num_trials = 1000
stay_rates, switch_rates = monty_hall_sim(num_trials)

# Visualization 1: Convergence Plot
plt.figure(figsize=(10, 6))
plt.plot(range(1, num_trials + 1), stay_rates, label='Stay Strategy', color='red')
plt.plot(range(1, num_trials + 1), switch_rates, label='Switch Strategy', color='green')
plt.axhline(y=1/3, color='black', linestyle='--', alpha=0.5, label='Theoretical Stay (1/3)')
plt.axhline(y=2/3, color='black', linestyle='-.', alpha=0.5, label='Theoretical Switch (2/3)')
plt.title(f'Monty Hall Simulation: Win Rates Over {num_trials} Trials')
plt.xlabel('Number of Trials')
plt.ylabel('Win Rate')
plt.legend()
plt.grid(True, linestyle=':', alpha=0.6)
plt.savefig('monty_hall_convergence.png')

# Visualization 2: Final Comparison Bar Chart
plt.figure(figsize=(8, 6))
labels = ['Stay', 'Switch']
wins = [stay_rates[-1], switch_rates[-1]]
colors = ['red', 'green']
bars = plt.bar(labels, wins, color=colors, alpha=0.7)
plt.ylim(0, 1)
plt.title('Final Win Probability Comparison')
plt.ylabel('Probability of Winning')

# Add text labels on top of bars
for bar in bars:
    yval = bar.get_height()
    plt.text(bar.get_x() + bar.get_width()/2, yval + 0.02, f'{yval:.2%}', ha='center', va='bottom', fontweight='bold')

plt.savefig('monty_hall_comparison.png')

print(f"Final Win Rate (Stay): {stay_rates[-1]:.7f}")
print(f"Final Win Rate (Switch): {switch_rates[-1]:.7f}")

