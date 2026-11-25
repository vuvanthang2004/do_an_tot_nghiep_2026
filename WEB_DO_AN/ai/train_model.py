
import pandas as pd
import pickle

from pathlib import Path

from sklearn.model_selection import train_test_split
from sklearn.ensemble import RandomForestClassifier

from sklearn.metrics import (
    accuracy_score,
    classification_report,
    confusion_matrix
)


# ==========================================
# 1. Xác định đường dẫn
# ==========================================

BASE_DIR = Path(__file__).resolve().parent

# dataset.csv nằm ngoài thư mục ai
DATASET_PATH = BASE_DIR.parent / "dataset.csv"

# File lưu mô hình AI
MODEL_PATH = BASE_DIR / "air_quality_model.pkl"


# ==========================================
# 2. Đọc dữ liệu
# ==========================================

df = pd.read_csv(DATASET_PATH)

print("Đọc dataset thành công!")
print("Số lượng mẫu:", len(df))

print("\n5 dòng dữ liệu đầu:")
print(df.head())


# ==========================================
# 3. Chuẩn bị dữ liệu
# ==========================================

# Các đặc trưng đầu vào của AI
features = [
    "temperature",
    "humidity",
    "mq2",
    "pm25"
]

# X: dữ liệu đầu vào
X = df[features]

# y: nhãn cần dự đoán
y = df["air_quality"]


# ==========================================
# 4. Chia dữ liệu train và test
# ==========================================

X_train, X_test, y_train, y_test = train_test_split(
    X,
    y,
    test_size=0.2,
    random_state=42,
    stratify=y
)

print("\nSố mẫu huấn luyện:", len(X_train))
print("Số mẫu kiểm tra:", len(X_test))


# ==========================================
# 5. Khởi tạo mô hình Random Forest
# ==========================================

model = RandomForestClassifier(
    n_estimators=100,
    random_state=42,
    class_weight="balanced"
)


# ==========================================
# 6. Huấn luyện AI
# ==========================================

print("\nĐang huấn luyện mô hình...")

model.fit(X_train, y_train)

print("Huấn luyện hoàn tất!")


# ==========================================
# 7. Đánh giá mô hình
# ==========================================

y_pred = model.predict(X_test)

accuracy = accuracy_score(y_test, y_pred)

print("\nAccuracy:", round(accuracy * 100, 2), "%")

print("\nClassification Report:")
print(
    classification_report(
        y_test,
        y_pred,
        zero_division=0
    )
)

print("\nConfusion Matrix:")
print(confusion_matrix(y_test, y_pred))


# ==========================================
# 8. Lưu mô hình
# ==========================================

with open(MODEL_PATH, "wb") as file:
    pickle.dump(model, file)

print("\nĐã lưu mô hình AI thành công!")
print("File mô hình:", MODEL_PATH)
