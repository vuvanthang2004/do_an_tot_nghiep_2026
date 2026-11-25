
import os
import sqlite3
import hashlib
import secrets
import smtplib

from datetime import datetime, timedelta, timezone
from email.message import EmailMessage
from pathlib import Path

from fastapi import APIRouter, HTTPException, Depends
from fastapi.security import HTTPBearer, HTTPAuthorizationCredentials
from pydantic import BaseModel, EmailStr, Field
from backend.database.database import get_connection
from mysql.connector.errors import IntegrityError


# =========================================================
# CẤU HÌNH
# =========================================================

router = APIRouter(
    prefix="/api/auth",
    tags=["Authentication"]
)

security = HTTPBearer()



# SMTP Gmail: cấu hình bằng biến môi trường
SMTP_HOST = os.getenv("SMTP_HOST", "smtp.gmail.com")
SMTP_PORT = int(os.getenv("SMTP_PORT", "587"))
SMTP_EMAIL = os.getenv("SMTP_EMAIL", "")
SMTP_PASSWORD = os.getenv("SMTP_PASSWORD", "")

# URL trang reset mật khẩu của Frontend
RESET_URL = os.getenv(
    "RESET_URL",
    "http://127.0.0.1:5500/reset-password.html"
)







# =========================================================
# GỬI EMAIL KHÔI PHỤC
# =========================================================

def send_reset_email(email: str, reset_link: str):
    if not SMTP_EMAIL or not SMTP_PASSWORD:
        raise RuntimeError(
            "Chưa cấu hình SMTP_EMAIL và SMTP_PASSWORD"
        )

    message = EmailMessage()

    message["Subject"] = "Khôi phục mật khẩu"
    message["From"] = SMTP_EMAIL
    message["To"] = email

    message.set_content(
        f"""
Xin chào,

Bạn đã yêu cầu đặt lại mật khẩu.

Hãy truy cập liên kết dưới đây để tạo mật khẩu mới:

{reset_link}

Liên kết có hiệu lực trong 15 phút.

Nếu bạn không yêu cầu khôi phục mật khẩu,
hãy bỏ qua email này.
"""
    )

    with smtplib.SMTP(SMTP_HOST, SMTP_PORT) as server:
        server.starttls()
        server.login(SMTP_EMAIL, SMTP_PASSWORD)
        server.send_message(message)


# =========================================================
# MODELS - DỮ LIỆU FRONTEND GỬI LÊN
# =========================================================

class RegisterRequest(BaseModel):
    fullname: str = Field(min_length=1, max_length=100)
    username: str = Field(min_length=3, max_length=50)
    email: EmailStr
    password: str = Field(min_length=8, max_length=128)


class LoginRequest(BaseModel):
    username: str
    password: str


class ForgotPasswordRequest(BaseModel):
    email: EmailStr


class ResetPasswordRequest(BaseModel):
    token: str
    new_password: str = Field(min_length=8, max_length=128)


# =========================================================
# ĐĂNG KÝ
# POST /api/auth/register
# =========================================================
# Dữ liệu nhận từ Frontend
class RegisterRequest(BaseModel):
    fullname: str = Field(min_length=1, max_length=100)
    username: str = Field(min_length=3, max_length=30)
    email: str = Field(min_length=1, max_length=255)
    password: str = Field(min_length=8)

@router.post("/register", status_code=201)
def register(data: RegisterRequest):

    fullname = data.fullname.strip()
    username = data.username.strip()
    email = data.email.strip().lower()
    password = data.password

    # Kiểm tra họ và tên
    if not fullname:
        raise HTTPException(
            status_code=400,
            detail="Họ và tên không được để trống"
        )

    # Kiểm tra username
    if not username.isascii() or not all(
        c.isalnum() or c == "_" for c in username
    ):
        raise HTTPException(
            status_code=400,
            detail="Username chỉ được chứa chữ, số và dấu gạch dưới"
        )

    # Kiểm tra email
    if "@" not in email or "." not in email.split("@")[-1]:
        raise HTTPException(
            status_code=400,
            detail="Email không hợp lệ"
        )

    conn = get_connection()
    cursor = conn.cursor(dictionary=True)

    try:
        # Kiểm tra username hoặc email đã tồn tại
        sql = """
            SELECT id, username, email
            FROM users
            WHERE username = %s OR email = %s
        """

        cursor.execute(sql, (username, email))
        existing_user = cursor.fetchone()

        if existing_user:

            if existing_user["username"] == username:
                raise HTTPException(
                    status_code=409,
                    detail="Username đã tồn tại"
                )

            if existing_user["email"] == email:
                raise HTTPException(
                    status_code=409,
                    detail="Email đã được đăng ký"
                )

        # Thêm tài khoản mới vào MySQL
        insert_sql = """
            INSERT INTO users (
                fullname,
                username,
                email,
                password_hash
            )
            VALUES (%s, %s, %s, %s)
        """

        cursor.execute(
            insert_sql,
            (fullname, username, email, password)
        )

        conn.commit()

        return {
            "status": "success",
            "message": "Đăng ký tài khoản thành công!",
            "user": {
                "id": cursor.lastrowid,
                "fullname": fullname,
                "username": username,
                "email": email
            }
        }

    except IntegrityError:
        conn.rollback()

        raise HTTPException(
            status_code=409,
            detail="Username hoặc email đã được đăng ký"
        )

    except HTTPException:
        conn.rollback()
        raise

    except Exception:
        conn.rollback()

        raise HTTPException(
            status_code=500,
            detail="Lỗi hệ thống khi đăng ký tài khoản"
        )

    finally:
        cursor.close()
        conn.close()


# =========================================================
# ĐĂNG NHẬP
# POST /api/auth/login
# =========================================================
class LoginRequest(BaseModel):
    username: str
    password: str

@router.post("/login")


def login(data: LoginRequest):

    username = data.username.strip()
    password = data.password

    conn = get_connection()
    cursor = conn.cursor(dictionary=True)

    try:
        sql = """
            SELECT id, fullname, username, email, password
            FROM users
            WHERE username = %s OR email = %s
        """

        cursor.execute(sql, (username, username))
        user = cursor.fetchone()

        if user is None:
            raise HTTPException(
                status_code=401,
                detail="Tài khoản không tồn tại"
            )

        if password != user["password"]:
            raise HTTPException(
                status_code=401,
                detail="Mật khẩu không đúng"
            )

        return {
            "status": "success",
            "message": "Đăng nhập thành công!",
            "username": user["username"]
        }

    finally:
        cursor.close()
        conn.close()


# =========================================================
# XÁC THỰC SESSION
# =========================================================

def get_current_user(
    credentials: HTTPAuthorizationCredentials = Depends(security)
):

    token = credentials.credentials
    token_hash = hash_token(token)

    conn = get_db()

    try:
        session = conn.execute(
            """
            SELECT users.id, users.username, users.email,
                   users.fullname, sessions.expires_at
            FROM sessions
            JOIN users ON users.id = sessions.user_id
            WHERE sessions.token_hash = ?
            """,
            (token_hash,)
        ).fetchone()

        if not session:
            raise HTTPException(
                status_code=401,
                detail="Phiên đăng nhập không hợp lệ"
            )

        expires_at = datetime.fromisoformat(
            session["expires_at"]
        )

        if utc_now() >= expires_at:
            conn.execute(
                "DELETE FROM sessions WHERE token_hash = ?",
                (token_hash,)
            )
            conn.commit()

            raise HTTPException(
                status_code=401,
                detail="Phiên đăng nhập đã hết hạn"
            )

        return {
            "id": session["id"],
            "username": session["username"],
            "email": session["email"],
            "fullname": session["fullname"]
        }

    finally:
        conn.close()


# =========================================================
# LẤY THÔNG TIN NGƯỜI ĐANG ĐĂNG NHẬP
# GET /api/auth/me
# =========================================================

@router.get("/me")
def get_me(user=Depends(get_current_user)):

    return {
        "success": True,
        "user": user
    }


# =========================================================
# QUÊN MẬT KHẨU
# POST /api/auth/forgot-password
# =========================================================

@router.post("/forgot-password")
def forgot_password(data: ForgotPasswordRequest):

    email = str(data.email).lower().strip()

    conn = get_db()

    try:
        user = conn.execute(
            "SELECT id FROM users WHERE email = ?",
            (email,)
        ).fetchone()

        # Không tiết lộ email có tồn tại hay không
        if not user:
            return {
                "success": True,
                "message": (
                    "Nếu email tồn tại, "
                    "hướng dẫn khôi phục sẽ được gửi."
                )
            }

        # Xóa token reset cũ của tài khoản
        conn.execute(
            "DELETE FROM reset_tokens WHERE user_id = ?",
            (user["id"],)
        )

        token = create_token()
        token_hash = hash_token(token)

        expires_at = utc_now() + timedelta(minutes=15)

        conn.execute(
            """
            INSERT INTO reset_tokens(token_hash, user_id, expires_at)
            VALUES (?, ?, ?)
            """,
            (
                token_hash,
                user["id"],
                expires_at.isoformat()
            )
        )

        conn.commit()

        reset_link = f"{RESET_URL}?token={token}"

        try:
            send_reset_email(email, reset_link)

        except Exception:
            # Không để token reset còn hiệu lực nếu email gửi thất bại
            conn.execute(
                "DELETE FROM reset_tokens WHERE token_hash = ?",
                (token_hash,)
            )
            conn.commit()

            raise HTTPException(
                status_code=500,
                detail="Không gửi được email. Kiểm tra cấu hình SMTP."
            )

        return {
            "success": True,
            "message": (
                "Nếu email tồn tại, "
                "hướng dẫn khôi phục sẽ được gửi."
            )
        }

    finally:
        conn.close()


# =========================================================
# ĐẶT LẠI MẬT KHẨU
# POST /api/auth/reset-password
# =========================================================

@router.post("/reset-password")
def reset_password(data: ResetPasswordRequest):

    token_hash = hash_token(data.token)

    conn = get_db()

    try:
        reset_record = conn.execute(
            """
            SELECT user_id, expires_at
            FROM reset_tokens
            WHERE token_hash = ?
            """,
            (token_hash,)
        ).fetchone()

        if not reset_record:
            raise HTTPException(
                status_code=400,
                detail="Token không hợp lệ hoặc đã được sử dụng"
            )

        expires_at = datetime.fromisoformat(
            reset_record["expires_at"]
        )

        if utc_now() >= expires_at:
            conn.execute(
                "DELETE FROM reset_tokens WHERE token_hash = ?",
                (token_hash,)
            )
            conn.commit()

            raise HTTPException(
                status_code=400,
                detail="Token đã hết hạn"
            )

        new_password_hash = hash_password(data.new_password)

        conn.execute(
            """
            UPDATE users
            SET password_hash = ?
            WHERE id = ?
            """,
            (
                new_password_hash,
                reset_record["user_id"]
            )
        )

        # Token reset chỉ được dùng một lần
        conn.execute(
            "DELETE FROM reset_tokens WHERE token_hash = ?",
            (token_hash,)
        )

        # Thu hồi toàn bộ session cũ của người dùng
        conn.execute(
            "DELETE FROM sessions WHERE user_id = ?",
            (reset_record["user_id"],)
        )

        conn.commit()

        return {
            "success": True,
            "message": "Đặt lại mật khẩu thành công"
        }

    finally:
        conn.close()


# =========================================================
# ĐĂNG XUẤT
# POST /api/auth/logout
# =========================================================

@router.post("/logout")
def logout(
    credentials: HTTPAuthorizationCredentials = Depends(security)
):

    token_hash = hash_token(credentials.credentials)

    conn = get_db()

    try:
        conn.execute(
            "DELETE FROM sessions WHERE token_hash = ?",
            (token_hash,)
        )

        conn.commit()

        return {
            "success": True,
            "message": "Đăng xuất thành công"
        }

    finally:
        conn.close()
