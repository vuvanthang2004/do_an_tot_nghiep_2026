
import pandas as pd
from pathlib import Path

# Đường dẫn dataset.csv nằm cùng thư mục
file_path = Path(__file__).parent / "dataset.csv"

# Đọc dữ liệu CSV
df = pd.read_csv(file_path, encoding="utf-8-sig")

print("========== KIEM TRA DATASET ==========")

# 1. Kiểm tra số lượng mẫu
print("Số mẫu:", len(df))

# 2. Kiểm tra các cột
print("\nCác cột dữ liệu:")
print(df.columns.tolist())

# 3. Kiểm tra dữ liệu thiếu
print("\nSố dữ liệu thiếu:")
print(df.isnull().sum())

# 4. Kiểm tra phân bố nhãn
print("\nPhân bố nhãn:")
print(df["air_quality"].value_counts())

# 5. Kiểm tra dữ liệu trùng
print("\nSố dòng trùng:", df.duplicated().sum())

# 6. Hiển thị 5 dòng đầu
print("\n5 dòng đầu:")
print(df.head())

print("\nDataset kiểm tra xong!")
