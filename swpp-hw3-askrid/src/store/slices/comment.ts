import { createAsyncThunk, createSlice } from "@reduxjs/toolkit";
import axios from "axios";
import { RootState } from "..";
import { API_URL_COMMENTS, API_URL_USER } from "../../constants";
import { NotLoggedInError, useCachedFetcher } from "../utils";
import { deleteArticle } from "./article";
import { UserClaims, UserData } from "./auth";

export interface CommentState {
  commentList: Comment[];
}

export interface Comment {
  id: number;
  articleId: number;
  author: UserClaims;
  content: string;
}

export interface CommentData {
  id: number;
  article_id: number;
  author_id: number;
  content: string;
}

const initialState: CommentState = {
  commentList: [],
};

const commentSlice = createSlice({
  name: "comment",
  initialState,
  reducers: {},
  extraReducers: ({ addCase }) => {
    // Fetch comment list
    addCase(fetchCommentList.fulfilled, (state, { payload }) => {
      state.commentList = payload;
    });
    addCase(fetchCommentList.pending, (state, { meta }) => {
      state.commentList.filter((comment) => comment.articleId === meta.arg);
    });

    // Create comment
    addCase(createComment.fulfilled, (state, { payload }) => {
      // This may not be robust enough, but it works.
      if (state.commentList.length === 0 || state.commentList[0].articleId === payload.articleId) {
        state.commentList.push(payload);
      }
    });

    // Update comment
    addCase(updateComment.fulfilled, (state, { payload }) => {
      state.commentList = state.commentList.map((comment) => {
        if (comment.id === payload.id) {
          return payload;
        } else {
          return comment;
        }
      });
    });

    // Delete comment
    addCase(deleteComment.fulfilled, (state, { payload }) => {
      state.commentList = state.commentList.filter((comment) => comment.id !== payload);
    });

    // Delete comments posted on the article.
    addCase(deleteArticle.fulfilled, (state, { payload }) => {
      state.commentList = state.commentList.filter((comment) => comment.articleId !== payload);
    });
  },
});

/**
 * Fetch the list of comments from the server.
 */
export const fetchCommentList = createAsyncThunk<Comment[], number>(
  "comment/fetchCommentList",
  async (articleId: number) => {
    const { data: comments } = await axios.get<CommentData[]>(`${API_URL_COMMENTS}/?article_id=${articleId}`);

    const getCachedUser = useCachedFetcher<UserData>();

    return await Promise.all(
      comments.map<Promise<Comment>>(async (comment) => {
        const user = await getCachedUser(`${API_URL_USER}/${comment.author_id}`);

        return {
          id: comment.id,
          articleId: comment.article_id,
          author: { id: user.id, email: user.email, name: user.name },
          content: comment.content,
        };
      })
    );
  }
);

/**
 * Create a comment on the server.
 */
export const createComment = createAsyncThunk<
  Comment,
  Pick<Comment, "articleId" | "content">,
  { state: RootState }
>("comment/createComment", async ({ articleId, content }, { getState }) => {
  const { claims, loginStatus } = getState().auth;

  // User is not logged in.
  if (loginStatus !== "LOGGED_IN" || !claims) {
    throw NotLoggedInError;
  }

  // Post the comment.
  const { data: comment } = await axios.post<CommentData>(`${API_URL_COMMENTS}`, {
    article_id: articleId,
    author_id: claims.id,
    content,
  });

  return {
    id: comment.id,
    articleId: comment.article_id,
    author: claims,
    content: comment.content,
  };
});

/**
 * Update a comment on the server
 */
export const updateComment = createAsyncThunk<Comment, Pick<Comment, "id" | "content">, { state: RootState }>(
  "comment/updateComment",
  async ({ id, content }, { getState }) => {
    const { claims, loginStatus } = getState().auth;

    // User is not logged in.
    if (loginStatus !== "LOGGED_IN" || !claims) {
      throw NotLoggedInError;
    }

    // Patch the comment.
    const { data: comment } = await axios.patch<CommentData>(`${API_URL_COMMENTS}/${id}`, { content });

    return {
      id: comment.id,
      articleId: comment.article_id,
      author: claims,
      content: comment.content,
    };
  }
);

/**
 * Delete a comment on the server.
 */
export const deleteComment = createAsyncThunk<number, number, { state: RootState }>(
  "comment/deleteComment",
  async (commentId, { getState }) => {
    const { claims, loginStatus } = getState().auth;

    // User is not logged in.
    if (loginStatus !== "LOGGED_IN" || !claims) {
      throw NotLoggedInError;
    }

    // Delete the comment.
    await axios.delete(`${API_URL_COMMENTS}/${commentId}`);

    return commentId;
  }
);

export const commentActions = commentSlice.actions;
export const commentReducer = commentSlice.reducer;
export const getCommentInitialState = commentSlice.getInitialState;
export const selectComment = (state: RootState) => state.comment;
