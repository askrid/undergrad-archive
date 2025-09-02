import { createAsyncThunk, createSlice } from "@reduxjs/toolkit";
import axios from "axios";
import { RootState } from "..";
import { API_URL_USER_REFERENCE } from "../../constants";

export interface AuthState {
  claims: UserClaims | null;
  loginStatus: "LOGGED_IN" | "LOGGED_OUT" | "LOADING";
}

export interface UserClaims {
  id: number;
  email: string;
  name: string;
}

export interface UserData {
  id: number;
  email: string;
  password: string;
  name: string;
  logged_in: boolean;
}

const initialState: AuthState = {
  claims: null,
  loginStatus: "LOADING",
};

const authSlice = createSlice({
  name: "auth",
  initialState,
  reducers: {},
  extraReducers: ({ addCase }) => {
    // Auth status
    addCase(getAuth.fulfilled, (state, { payload }) => {
      state.claims = payload.claims;
      state.loginStatus = payload.loginStatus;
    });

    // Login
    addCase(login.fulfilled, (state, { payload }) => {
      // Modify the state only when login is successful.
      if (payload) {
        state.claims = payload;
        state.loginStatus = "LOGGED_IN";
      }
      // If not successful, just persist the state.
    });

    // Logout
    addCase(logout.fulfilled, (state) => {
      state.claims = null;
      state.loginStatus = "LOGGED_OUT";
    });
  },
});

/**
 * Get the current user's state from the server.
 */
export const getAuth = createAsyncThunk<AuthState, void>("user/getAuth", async () => {
  // Fetch the reference user's data from the database in order to sync the login status.
  const { data: refUser } = await axios.get<UserData>(API_URL_USER_REFERENCE);

  if (refUser.logged_in) {
    return { claims: { id: refUser.id, email: refUser.email, name: refUser.name }, loginStatus: "LOGGED_IN" };
  } else {
    return { claims: null, loginStatus: "LOGGED_OUT" };
  }
});

/**
 * Login thunk
 */
export const login = createAsyncThunk<UserClaims | null, { email: string; password: string }>(
  "auth/login",
  async ({ email, password }) => {
    // Get reference user data with credentials.
    const { data: refUser } = await axios.get<UserData>(API_URL_USER_REFERENCE);

    if (email === refUser.email && password === refUser.password) {
      // Set reference user's 'logged_in' to true.
      await axios.patch<UserData>(API_URL_USER_REFERENCE, { logged_in: true });
      return { id: refUser.id, email: refUser.email, name: refUser.name };
    } else {
      return null;
    }
  }
);

/**
 * Logout thunk
 */
export const logout = createAsyncThunk<void, void>("auth/logout", async () => {
  // Set the reference user's 'logged_out' to false.
  await axios.patch<UserData>(API_URL_USER_REFERENCE, { logged_in: false });
});

export const authActions = authSlice.actions;
export const authReducer = authSlice.reducer;
export const getAuthInitialState = authSlice.getInitialState;
export const selectAuth = (state: RootState) => state.auth;
