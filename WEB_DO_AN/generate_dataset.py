
import pandas as pd
import numpy as np
from pathlib import Path

# Đảm bảo kết quả có thể tái tạo
np.random.seed(42)

# Số lượng mẫu
N_GOOD = 1700
N_MODERATE = 1700
N_POOR = 1600

# Hàm tạo dữ liệu mô phỏng
def generate_data(n, pm25_range, mq2_range, temperature_range, humidity_range, label):

    data = {
        "temperature": np.random.uniform(
            *temperature_range, n
        ),

        "humidity": np.random.uniform(
            *humidity_range, n
        ),

        "mq2": np.random.uniform(
            *mq2_range, n
        ),

        "pm25": np.random.uniform(
            *pm25_range, n
        ),

        "air_quality": [label] * n
    }

    return pd.DataFrame(data)


# 1. Không khí tốt
good = generate_data(
    N_GOOD,
    (0, 15),
    (0, 30),
    (22, 29),
    (40, 65),
    "good"
)

# 2. Không khí trung bình
moderate = generate_data(
    N_MODERATE,
    (15.1, 35),
    (30, 60),
    (25, 33),
    (50, 75),
    "moderate"
)

# 3. Không khí kém
poor = generate_data(
    N_POOR,
    (35.1, 100),
    (60, 100),
    (28, 38),
    (60, 90),
    "poor"
)

# Kết hợp
df = pd.concat(
    [good, moderate, poor],
    ignore_index=True
)

# Xáo trộn dữ liệu
df = df.sample(
    frac=1,
    random_state=42
).reset_index(drop=True)

# Thêm timestamp mô phỏng
df.insert(
    0,
    "timestamp",
    pd.date_range(
        start="2026-01-01",
        periods=len(df),
        freq="1min"
    )
)

# Lưu dataset
output_path = Path(__file__).parent / "dataset.csv"

df.to_csv(
    output_path,
    index=False,
    encoding="utf-8-sig"
)

print("Da tao dataset thanh cong!")
print("So dong:", len(df))
print("\nPhan bo nhan:")
print(df["air_quality"].value_counts())
print("\n5 dong dau:")
print(df.head())
print("\nFile:", output_path)
