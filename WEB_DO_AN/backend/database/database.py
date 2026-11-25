
import mysql.connector
from mysql.connector import Error


def get_connection():
    """Kết nối Backend Python với MySQL Server."""

    try:
        conn = mysql.connector.connect(
            # Host dùng để xác định máy tính đang chạy MySQL Server mà Workbench hoặc Backend muốn kết nối đến
            host="localhost",
            port=3306,
            user="root",
            password="root",
            database="air_quality_db"
        )

        return conn

    except Error as e:
        print("Lỗi kết nối MySQL:", e)
        raise
