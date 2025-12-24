import streamlit as st
import socket
import struct
from typing import Optional, List, Tuple
import time

st.set_page_config(
    page_title="MovieFlixITU - Your Personal Movie Guide",
    page_icon="🎬",
    layout="wide",
    initial_sidebar_state="expanded"
)

st.markdown("""
<style>
    /* Hide Streamlit branding */
    #MainMenu {visibility: hidden;}
    footer {visibility: hidden;}
    header {visibility: hidden;}
    
    /* Main background */
    .stApp {
        background-color: #0f0f0f;
    }
    
    /* Sidebar styling */
    [data-testid="stSidebar"] {
        background-color: #1a1a1a;
        border-right: 1px solid #2a2a2a;
    }
    
    [data-testid="stSidebar"] [data-testid="stMarkdownContainer"] p {
        color: #ffffff;
    }
    
    /* Title styling */
    h1 {
        color: #9146ff !important;
        font-weight: 700 !important;
        padding-bottom: 10px;
        font-size: 1.8em !important;
        margin-bottom: 10px !important;
    }
    
    h2, h3 {
        color: #ffffff !important;
        font-weight: 600 !important;
        margin-bottom: 8px !important;
    }
    
    /* Movie card container */
    .movie-card {
        background: linear-gradient(135deg, #1a1a1a 0%, #252525 100%);
        border: 1px solid #3a3a3a;
        border-radius: 6px;
        padding: 12px 16px;
        margin: 8px 0;
        transition: all 0.3s ease;
        box-shadow: 0 2px 8px rgba(0,0,0,0.3);
    }
    
    .movie-card:hover {
        border-color: #9146ff;
        box-shadow: 0 4px 16px rgba(145, 70, 255, 0.3);
        transform: translateY(-2px);
    }
    
    .movie-title {
        color: #ffffff;
        font-size: 1.05em;
        font-weight: 600;
        margin-bottom: 4px;
        line-height: 1.3;
    }
    
    .movie-genres {
        color: #888888;
        font-size: 0.85em;
        margin-top: 3px;
    }
    
    .rating-badge {
        background-color: #9146ff;
        color: #ffffff;
        padding: 4px 10px;
        border-radius: 4px;
        font-weight: 700;
        font-size: 0.95em;
        display: inline-block;
    }
    
    .match-score {
        background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
        color: #ffffff;
        padding: 4px 10px;
        border-radius: 4px;
        font-weight: 700;
        font-size: 0.95em;
        display: inline-block;
    }
    
    /* Button styling */
    .stButton > button {
        background: linear-gradient(135deg, #9146ff 0%, #772ce8 100%);
        color: #ffffff;
        font-weight: 600;
        border: none;
        border-radius: 6px;
        padding: 8px 16px;
        transition: all 0.3s ease;
        font-size: 0.95em;
    }
    
    .stButton > button:hover {
        background: linear-gradient(135deg, #772ce8 0%, #6441a5 100%);
        box-shadow: 0 4px 12px rgba(145, 70, 255, 0.5);
        transform: translateY(-1px);
    }
    
    /* Input fields */
    .stTextInput > div > div > input {
        background-color: #2a2a2a;
        color: #ffffff;
        border: 1px solid #3a3a3a;
        border-radius: 6px;
    }
    
    .stTextInput > div > div > input:focus {
        border-color: #9146ff;
        box-shadow: 0 0 0 1px #9146ff;
    }
    
    /* Slider */
    .stSlider > div > div > div {
        background-color: #9146ff;
    }
    
    /* Info/Success/Warning boxes */
    .stAlert {
        background-color: #2a2a2a;
        border: 1px solid #3a3a3a;
        border-radius: 6px;
    }
    
    /* Metric styling */
    [data-testid="stMetricValue"] {
        color: #9146ff;
        font-size: 1.5em;
    }
    
    /* Radio buttons */
    .stRadio > label {
        color: #ffffff;
    }
    
    /* Divider */
    hr {
        border-color: #3a3a3a;
        margin: 15px 0;
    }
    
    /* Selectbox */
    .stSelectbox > div > div {
        background-color: #2a2a2a;
        color: #ffffff;
        border: 1px solid #3a3a3a;
    }
    
    /* Text color */
    p, span, label {
        color: #cccccc;
    }
    
    /* Logo styling */
    .logo {
        color: #9146ff;
        font-size: 1.6em;
        font-weight: 800;
        text-align: center;
        padding: 5px 0;
        letter-spacing: 1px;
        margin-bottom: 5px;
    }
</style>
""", unsafe_allow_html=True)

# Message types
LOGIN = 1
REGISTER = 2
GET_RECOMMENDATIONS = 3
SEARCH_MOVIES = 4
ADD_RATING = 5
GET_POPULAR = 6
GET_USER_RATINGS = 7
GET_MOVIE_DETAILS = 8
GET_ALL_GENRES = 10
GET_MOVIES_BY_GENRE = 11
CHANGE_PASSWORD = 12
GET_COLD_START = 13
LOGOUT = 14
SUCCESS = 100
ERROR = 101

class MovieClient:
    def __init__(self, host='127.0.0.1', port=8080):
        self.host = host
        self.port = port
        self.sock = None
        
    def connect(self):
        try:
            self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.sock.connect((self.host, self.port))
            return True
        except Exception as e:
            st.error(f"Connection failed: {e}")
            return False
    
    def send_message(self, msg_type, user_id, data):
        if not self.sock:
            if not self.connect():
                return None
        
        try:
            msg = struct.pack('i', msg_type)
            msg += struct.pack('i', user_id)
            msg += data.encode('utf-8').ljust(8192, b'\x00')
            
            self.sock.sendall(msg)
            
            response = self.sock.recv(8200)
            resp_type = struct.unpack('i', response[:4])[0]
            resp_user_id = struct.unpack('i', response[4:8])[0]
            resp_data = response[8:].rstrip(b'\x00').decode('utf-8', errors='ignore')
            
            return resp_type, resp_user_id, resp_data
        except Exception as e:
            st.error(f"Communication error: {e}")
            self.sock = None
            return None
    
    def close(self):
        if self.sock:
            try:
                self.sock.close()
            except:
                pass
            self.sock = None

def init_session_state():
    if 'logged_in' not in st.session_state:
        st.session_state.logged_in = False
    if 'user_id' not in st.session_state:
        st.session_state.user_id = 0
    if 'username' not in st.session_state:
        st.session_state.username = ''
    if 'client' not in st.session_state:
        st.session_state.client = MovieClient()

def login_page():
    col1, col2, col3 = st.columns([1, 2, 1])
    
    with col2:
        st.markdown('<div class="logo">🎬 MovieFlix</div>', unsafe_allow_html=True)
        st.markdown("### Welcome Back")
        st.markdown("---")
        
        username = st.text_input("Username", key="login_username", placeholder="Enter your username")
        password = st.text_input("Password", type="password", key="login_password", placeholder="Enter your password")
        
        st.markdown("")
        col_a, col_b = st.columns(2)
        
        with col_a:
            if st.button("Sign In", type="primary", use_container_width=True):
                if username and password:
                    client = st.session_state.client
                    data = f"{username}|{password}"
                    result = client.send_message(LOGIN, 0, data)
                    
                    if result:
                        resp_type, user_id, resp_data = result
                        if resp_type == SUCCESS:
                            st.session_state.logged_in = True
                            st.session_state.user_id = user_id
                            st.session_state.username = username
                            st.success("✓ Login successful!")
                            time.sleep(0.5)
                            st.rerun()
                        else:
                            st.error(f"✗ {resp_data}")
                else:
                    st.warning("Please enter username and password")
        
        with col_b:
            if st.button("Create Account", use_container_width=True):
                st.session_state.show_register = True
                st.rerun()

def register_page():
    col1, col2, col3 = st.columns([1, 2, 1])
    
    with col2:
        st.markdown('<div class="logo">🎬 MovieFlix</div>', unsafe_allow_html=True)
        st.markdown("### Create Your Account")
        st.markdown("---")
        
        username = st.text_input("Username", placeholder="Choose a username (3-63 characters)")
        password = st.text_input("Password", type="password", placeholder="Minimum 6 characters")
        confirm = st.text_input("Confirm Password", type="password", placeholder="Re-enter your password")
        
        st.markdown("")
        col1, col2 = st.columns(2)
        
        with col1:
            if st.button("Register", type="primary", use_container_width=True):
                if not username or len(username) < 3:
                    st.error("✗ Username must be at least 3 characters")
                elif len(password) < 6:
                    st.error("✗ Password must be at least 6 characters")
                elif password != confirm:
                    st.error("✗ Passwords don't match")
                else:
                    client = st.session_state.client
                    data = f"{username}|{password}"
                    result = client.send_message(REGISTER, 0, data)
                    
                    if result:
                        resp_type, user_id, resp_data = result
                        if resp_type == SUCCESS:
                            st.success(f"✓ Account created! ID: {user_id}")
                            st.info("You can now sign in")
                            time.sleep(1)
                            st.session_state.show_register = False
                            st.rerun()
                        else:
                            st.error(f"✗ {resp_data}")
        
        with col2:
            if st.button("Back to Login", use_container_width=True):
                st.session_state.show_register = False
                st.rerun()

def main_page():
    # Sidebar
    st.sidebar.markdown('<div class="logo">🎬 MovieFlix</div>', unsafe_allow_html=True)
    st.sidebar.markdown(f"**Welcome, {st.session_state.username}**")
    st.sidebar.caption(f"User ID: {st.session_state.user_id}")
    st.sidebar.markdown("---")
    
    menu = st.sidebar.radio(
        "Navigate",
        ["🎯 For You", "⭐ Rate Movies", "📊 My Ratings", "🔍 Discover", "🔥 Trending"],
        label_visibility="collapsed"
    )
    
    st.sidebar.markdown("---")
    if st.sidebar.button("🚪 Sign Out", use_container_width=True):
        client = st.session_state.client
        client.send_message(LOGOUT, st.session_state.user_id, "")
        st.session_state.logged_in = False
        st.session_state.user_id = 0
        st.session_state.username = ''
        st.rerun()
    
    # Main content
    if menu == "🎯 For You":
        show_recommendations()
    elif menu == "⭐ Rate Movies":
        show_rate_movies()
    elif menu == "📊 My Ratings":
        show_my_ratings()
    elif menu == "🔍 Discover":
        show_search()
    elif menu == "🔥 Trending":
        show_popular()

def show_recommendations():
    st.title("🎯 Recommended For You")
    
    col1, col2 = st.columns([3, 1])
    with col1:
        top_n = st.slider("Number of recommendations", 5, 20, 10, label_visibility="collapsed")
    with col2:
        get_recs = st.button("🎬 Get Recommendations", type="primary", use_container_width=True)
    
    if get_recs:
        with st.spinner("🔍 Finding perfect matches..."):
            client = st.session_state.client
            result = client.send_message(GET_RECOMMENDATIONS, st.session_state.user_id, str(top_n))
            
            if result:
                resp_type, _, resp_data = result
                
                if resp_type == SUCCESS and resp_data:
                    lines = resp_data.strip().split('\n')
                    
                    if lines and lines[0]:
                        st.markdown("---")
                        for idx, line in enumerate(lines[:top_n], 1):
                            if not line:
                                continue
                            parts = line.split('|')
                            if len(parts) >= 5:
                                movie_id, title, score, avg_rating, genres = parts[0], parts[1], parts[2], parts[3], parts[4]
                                
                                st.markdown(f"""
                                <div class="movie-card">
                                    <div style="display: flex; justify-content: space-between; align-items: center;">
                                        <div style="flex: 1;">
                                            <div class="movie-title">{idx}. {title}</div>
                                            <div class="movie-genres">🎭 {genres}</div>
                                        </div>
                                        <div style="text-align: right; margin-left: 15px;">
                                            <div class="match-score">{float(score):.1f}% Match</div>
                                        </div>
                                    </div>
                                </div>
                                """, unsafe_allow_html=True)
                    else:
                        st.info("🎬 Building your taste profile... Here are some acclaimed films to start!")
                        result2 = client.send_message(GET_COLD_START, st.session_state.user_id, "")
                        
                        if result2:
                            resp_type2, _, resp_data2 = result2
                            if resp_type2 == SUCCESS:
                                lines2 = resp_data2.strip().split('\n')
                                st.markdown("---")
                                for idx, line in enumerate(lines2, 1):
                                    if not line:
                                        continue
                                    parts = line.split('|')
                                    if len(parts) >= 3:
                                        movie_id, title, rating = parts[0], parts[1], parts[2]
                                        genres = parts[3] if len(parts) > 3 else ""
                                        
                                        st.markdown(f"""
                                        <div class="movie-card">
                                            <div style="display: flex; justify-content: space-between; align-items: center;">
                                                <div style="flex: 1;">
                                                    <div class="movie-title">{idx}. {title}</div>
                                                    <div class="movie-genres">🎭 {genres}</div>
                                                </div>
                                                <div style="text-align: right; margin-left: 15px;">
                                                    <div class="rating-badge">⭐ {float(rating):.1f}</div>
                                                </div>
                                            </div>
                                        </div>
                                        """, unsafe_allow_html=True)

def show_rate_movies():
    st.title("⭐ Rate Movies")
    
    search_query = st.text_input("Search for a movie to rate", placeholder="Enter movie title...")
    
    if search_query:
        client = st.session_state.client
        result = client.send_message(SEARCH_MOVIES, st.session_state.user_id, search_query)
        
        if result:
            resp_type, _, resp_data = result
            if resp_type == SUCCESS and resp_data:
                lines = resp_data.strip().split('\n')
                
                st.markdown(f"**Found {len(lines)} movies**")
                st.markdown("---")
                
                for line in lines[:10]:
                    if not line:
                        continue
                    parts = line.split('|')
                    if len(parts) >= 2:
                        movie_id, title = parts[0], parts[1]
                        
                        col1, col2 = st.columns([4, 1])
                        with col1:
                            st.markdown(f'<div class="movie-title">{title}</div>', unsafe_allow_html=True)
                        with col2:
                            rating = st.selectbox(
                                "Rate",
                                ["-", "1.0", "2.0", "3.0", "4.0", "5.0"],
                                key=f"rate_{movie_id}",
                                label_visibility="collapsed"
                            )
                            if rating != "-":
                                data = f"{movie_id}|{rating}"
                                result = client.send_message(ADD_RATING, st.session_state.user_id, data)
                                if result and result[0] == SUCCESS:
                                    st.success("Done")
                        st.markdown('<hr style="margin: 10px 0; border-color: #2a2a2a;">', unsafe_allow_html=True)

def show_my_ratings():
    st.title("📊 My Ratings")
    
    client = st.session_state.client
    result = client.send_message(GET_USER_RATINGS, st.session_state.user_id, "")
    
    if result:
        resp_type, _, resp_data = result
        if resp_type == SUCCESS:
            if not resp_data or resp_data.strip() == "":
                st.info("You haven't rated any movies yet. Start rating to get personalized recommendations!")
            else:
                lines = resp_data.strip().split('\n')
                st.markdown(f"**Total: {len(lines)} ratings**")
                st.markdown("---")
                
                for line in lines:
                    if not line:
                        continue
                    parts = line.split('|')
                    if len(parts) >= 2:
                        movie_id, rating = parts[0], parts[1]
                        
                        detail_result = client.send_message(GET_MOVIE_DETAILS, st.session_state.user_id, movie_id)
                        
                        if detail_result and detail_result[0] == SUCCESS:
                            detail_parts = detail_result[2].split('|')
                            if len(detail_parts) >= 2:
                                title = detail_parts[1]
                                
                                st.markdown(f"""
                                <div class="movie-card">
                                    <div style="display: flex; justify-content: space-between; align-items: center;">
                                        <div style="flex: 1;">
                                            <div class="movie-title">{title}</div>
                                        </div>
                                        <div style="text-align: right; margin-left: 15px;">
                                            <div class="rating-badge">⭐ {float(rating):.1f}</div>
                                        </div>
                                    </div>
                                </div>
                                """, unsafe_allow_html=True)

def show_search():
    st.title("🔍 Discover Movies")
    
    search_type = st.radio("Search by", ["Title", "Genre"], horizontal=True)
    
    if search_type == "Title":
        query = st.text_input("Enter movie title", placeholder="Search by title...")
        
        if query:
            client = st.session_state.client
            result = client.send_message(SEARCH_MOVIES, st.session_state.user_id, query)
            
            if result:
                resp_type, _, resp_data = result
                if resp_type == SUCCESS and resp_data:
                    lines = resp_data.strip().split('\n')
                    st.success(f"✓ Found {len(lines)} movies")
                    st.markdown("---")
                    
                    for line in lines[:20]:
                        if not line:
                            continue
                        parts = line.split('|')
                        if len(parts) >= 2:
                            movie_id, title = parts[0], parts[1]
                            st.markdown(f"""
                            <div class="movie-card">
                                <div class="movie-title">{title}</div>
                                <div class="movie-genres">ID: {movie_id}</div>
                            </div>
                            """, unsafe_allow_html=True)
                else:
                    st.warning("No movies found")
    
    else:  # Genre
        client = st.session_state.client
        result = client.send_message(GET_ALL_GENRES, st.session_state.user_id, "")
        
        if result:
            resp_type, _, resp_data = result
            if resp_type == SUCCESS:
                genres = resp_data.strip().split('\n')
                selected_genre = st.selectbox("Select genre", genres)
                
                if st.button("🔍 Search", type="primary"):
                    result2 = client.send_message(GET_MOVIES_BY_GENRE, st.session_state.user_id, selected_genre)
                    
                    if result2:
                        resp_type2, _, resp_data2 = result2
                        if resp_type2 == SUCCESS:
                            movie_ids = resp_data2.strip().split('\n')
                            st.success(f"✓ Found {len(movie_ids)} {selected_genre} movies")
                            st.markdown("---")
                            
                            for movie_id in movie_ids[:20]:
                                if not movie_id:
                                    continue
                                
                                detail_result = client.send_message(GET_MOVIE_DETAILS, st.session_state.user_id, movie_id)
                                
                                if detail_result and detail_result[0] == SUCCESS:
                                    detail_parts = detail_result[2].split('|')
                                    if len(detail_parts) >= 3:
                                        title, avg_rating = detail_parts[1], detail_parts[2]
                                        st.markdown(f"""
                                        <div class="movie-card">
                                            <div style="display: flex; justify-content: space-between; align-items: center;">
                                                <div style="flex: 1;">
                                                    <div class="movie-title">{title}</div>
                                                </div>
                                                <div style="text-align: right; margin-left: 15px;">
                                                    <div class="rating-badge">⭐ {float(avg_rating):.1f}</div>
                                                </div>
                                            </div>
                                        </div>
                                        """, unsafe_allow_html=True)

def show_popular():
    st.title("🔥 Trending Now")
    
    with st.spinner("Loading trending titles..."):
        client = st.session_state.client
        result = client.send_message(GET_POPULAR, st.session_state.user_id, "15")
        
        if result:
            resp_type, _, resp_data = result
            if resp_type == SUCCESS:
                lines = resp_data.strip().split('\n')
                st.markdown("---")
                
                for idx, line in enumerate(lines, 1):
                    if not line:
                        continue
                    parts = line.split('|')
                    if len(parts) >= 4:
                        movie_id, title, rating, count = parts[0], parts[1], parts[2], parts[3]
                        
                        st.markdown(f"""
                        <div class="movie-card">
                            <div style="display: flex; justify-content: space-between; align-items: center;">
                                <div style="flex: 1;">
                                    <div class="movie-title">{idx}. {title}</div>
                                    <div class="movie-genres">👥 {count} ratings</div>
                                </div>
                                <div style="text-align: right; margin-left: 15px;">
                                    <div class="rating-badge">⭐ {float(rating):.1f}</div>
                                </div>
                            </div>
                        </div>
                        """, unsafe_allow_html=True)

# Initialize
init_session_state()

if 'show_register' not in st.session_state:
    st.session_state.show_register = False

# Routing
if not st.session_state.logged_in:
    if st.session_state.show_register:
        register_page()
    else:
        login_page()
else:
    main_page()