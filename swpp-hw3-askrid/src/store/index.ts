import { combineReducers, configureStore, PreloadedState } from "@reduxjs/toolkit";
import { articleReducer } from "./slices/article";
import { authReducer } from "./slices/auth";
import { commentReducer } from "./slices/comment";

const rootReducer = combineReducers({
  auth: authReducer,
  article: articleReducer,
  comment: commentReducer,
});

export const setupStore = (preloadedState?: PreloadedState<RootState>) =>
  configureStore({
    reducer: rootReducer,
    preloadedState,
  });

export type RootState = ReturnType<typeof rootReducer>;
export type AppStore = ReturnType<typeof setupStore>
export type AppDispatch = AppStore["dispatch"];
