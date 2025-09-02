import { createAsyncThunk, createSlice, PayloadAction } from "@reduxjs/toolkit";
import axios from "axios";
import { RootState } from "..";
import { API_URL_ARTICLES, API_URL_COMMENTS, API_URL_USER } from "../../constants";
import { NotLoggedInError, useCachedFetcher } from "../utils";
import { UserClaims, UserData } from "./auth";
import { CommentData } from "./comment";

export interface ArticleState {
  articleList: Article[];
  selectedArticle: Article | null;
  writingArticle: Pick<Article, "title" | "content">;
  writingArticleView: ArticleView;
}

export interface Article {
  id: number;
  title: string;
  author: UserClaims;
  content: string;
}

export type ArticleView = "WRITE" | "PREVIEW";

export interface ArticleData {
  id: number;
  author_id: number;
  title: string;
  content: string;
}

const initialState: ArticleState = {
  articleList: [],
  selectedArticle: null,
  writingArticle: { title: "", content: "" },
  writingArticleView: "WRITE",
};

const articleSlice = createSlice({
  name: "article",
  initialState,
  reducers: {
    // Modify writing article
    writeArticleTitle: (state, { payload }: PayloadAction<string>) => {
      state.writingArticle.title = payload;
    },
    writeArticleContent: (state, { payload }: PayloadAction<string>) => {
      state.writingArticle.content = payload;
    },
    clearWritingArticle: (state) => {
      state.writingArticle = { title: "", content: "" };
    },

    // Set writing article's view state to either "WRITE" or "PREVIEW".
    setWritingArticleView: (state, { payload }: PayloadAction<ArticleView>) => {
      state.writingArticleView = payload;
    },
  },
  extraReducers: ({ addCase }) => {
    // Fetch article list
    addCase(fetchArticleList.fulfilled, (state, { payload }) => {
      state.articleList = payload;
    });

    // Fetch selected article
    addCase(fetchSelectedArticle.fulfilled, (state, { payload }) => {
      state.selectedArticle = payload;
    });
    addCase(fetchSelectedArticle.pending, (state, { meta }) => {
      // Set selected article to null if fetching a different article.
      if (state.selectedArticle?.id !== meta.arg) {
        state.selectedArticle = null;
      }
    });

    // Create article
    addCase(createArticle.fulfilled, (state, { payload }) => {
      state.articleList.push(payload);
    });

    // Update article
    addCase(updateArticle.fulfilled, (state, { payload }) => {
      state.articleList = state.articleList.map((article) => {
        if (article.id === payload.id) {
          return payload;
        } else {
          return article;
        }
      });

      if (state.selectedArticle?.id === payload.id) {
        state.selectedArticle = payload;
      }
    });

    // Delete article
    addCase(deleteArticle.fulfilled, (state, { payload }) => {
      state.articleList = state.articleList.filter((article) => article.id !== payload);

      if (state.selectedArticle?.id === payload) {
        state.selectedArticle = null;
      }
    });
  },
});

/**
 * Fetch the list of articles from the server.
 */
export const fetchArticleList = createAsyncThunk<Article[], void>("article/fetchArticleList", async () => {
  const { data: articles } = await axios.get<ArticleData[]>(API_URL_ARTICLES);

  const getCachedUser = useCachedFetcher<UserData>();

  return await Promise.all<Article>(
    articles.map<Promise<Article>>(async (article) => {
      const user = await getCachedUser(`${API_URL_USER}/${article.author_id}`);

      return {
        id: article.id,
        title: article.title,
        author: { id: user.id, email: user.email, name: user.name },
        content: article.content,
      };
    })
  );
});

/**
 * Fetch an article from the server.
 */
export const fetchSelectedArticle = createAsyncThunk<Article, number>(
  "article/fetchSelectedArticle",
  async (articleId) => {
    const { data: article } = await axios.get<ArticleData>(`${API_URL_ARTICLES}/${articleId}`);
    const { data: user } = await axios.get<UserData>(`${API_URL_USER}/${article.author_id}`);

    return {
      id: article.id,
      title: article.title,
      author: { id: user.id, email: user.email, name: user.name },
      content: article.content,
    };
  }
);

/**
 * Create an article on the server.
 */
export const createArticle = createAsyncThunk<
  Article,
  Pick<Article, "title" | "content">,
  { state: RootState }
>("article/createArticle", async ({ title, content }, { getState }) => {
  const { claims, loginStatus } = getState().auth;

  // User is not logged in.
  if (loginStatus !== "LOGGED_IN" || !claims) {
    throw NotLoggedInError;
  }

  // Post the article.
  const { data: article } = await axios.post<ArticleData>(API_URL_ARTICLES, {
    author_id: claims.id,
    title,
    content,
  });

  return {
    id: article.id,
    title: article.title,
    author: claims,
    content: article.content,
  };
});

/**
 * Update an article on the server
 */
export const updateArticle = createAsyncThunk<
  Article,
  Pick<Article, "id" | "title" | "content">,
  { state: RootState }
>("article/updateArticle", async ({ id, title, content }, { getState }) => {
  const { claims, loginStatus } = getState().auth;

  // User is not logged in.
  if (loginStatus !== "LOGGED_IN" || !claims) {
    throw NotLoggedInError;
  }

  // Patch the comment.
  const { data: article } = await axios.patch<ArticleData>(`${API_URL_ARTICLES}/${id}`, { title, content });

  return {
    id: article.id,
    title: article.title,
    author: claims,
    content: article.content,
  };
});

/**
 * Delete an article in the server.
 */
export const deleteArticle = createAsyncThunk<number, number, { state: RootState }>(
  "article/deleteArticle",
  async (articleId, { getState }) => {
    const { claims, loginStatus } = getState().auth;

    // User is not logged in.
    if (loginStatus !== "LOGGED_IN" || !claims) {
      throw NotLoggedInError;
    }

    // Delete the article.
    await axios.delete(`${API_URL_ARTICLES}/${articleId}`);

    // Cascade deletion to the comments referencing the article, which should be a server logic.
    // Since json-server doesn't support queried delete, this might be very undesirabled implementation.
    const { data: comments } = await axios.get<CommentData[]>(`${API_URL_COMMENTS}/?article_id=${articleId}`);
    await Promise.all(comments.map((comment) => axios.delete(`${API_URL_COMMENTS}/${comment.id}`)));

    // Return article ID to use it on modifying the state.
    return articleId;
  }
);

export const articleActions = articleSlice.actions;
export const articleReducer = articleSlice.reducer;
export const getArtilceInitialState = articleSlice.getInitialState;
export const selectArticle = (state: RootState) => state.article;
