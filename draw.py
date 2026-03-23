import pandas as pd
import matplotlib.pyplot as plt

df = pd.read_csv("object_stats.csv")
plt.scatter(df['SIZE'], df['ACCESS_COUNT'], s=100, alpha=0.6)
plt.xlabel("Object Size")
plt.ylabel("Access Count")
plt.title("Object Size vs Access Count (point size = lifetime)")
plt.show()