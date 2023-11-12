import math



STEPS = 360

print("---\n")
for x in range(STEPS):
    val = math.sin(x * (math.pi / 180.0))
    print(f"{val:.3f}f,", end=" ")
print("\n\n---")

