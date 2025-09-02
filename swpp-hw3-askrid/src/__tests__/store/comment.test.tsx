import axios from "axios";
import { API_URL_COMMENTS } from "../../constants";
import { AppStore, setupStore } from "../../store";
import { deleteArticle } from "../../store/slices/article";
import { UserData } from "../../store/slices/auth";
import {
  CommentData,
  createComment,
  deleteComment,
  fetchCommentList,
  updateComment,
} from "../../store/slices/comment";

const stubCommentData: CommentData = {
  id: 1,
  article_id: 1,
  author_id: 1,
  content: "foo1",
};
const stubCommentDataList: CommentData[] = [
  stubCommentData,
  { id: 2, author_id: 1, article_id: 1, content: "foo2" },
  { id: 3, author_id: 1, article_id: 1, content: "foo3" },
  { id: 4, author_id: 1, article_id: 1, content: "foo4" },
];
const stubUserData: UserData = {
  id: 1,
  email: "test@example.com",
  name: "john",
  password: "password",
  logged_in: true,
};

describe("comment slice", () => {
  let store: AppStore;
  let loggedOutStore: AppStore;

  beforeEach(() => {
    store = setupStore({
      auth: { claims: { id: 1, email: "test@example.com", name: "john" }, loginStatus: "LOGGED_IN" },
      comment: {
        commentList: [
          {
            id: 1,
            articleId: 1,
            author: { id: 1, email: "test@example.com", name: "john" },
            content: "foo1",
          },
          {
            id: 2,
            articleId: 1,
            author: { id: 1, email: "test@example.com", name: "john" },
            content: "foo2",
          },
          {
            id: 3,
            articleId: 1,
            author: { id: 1, email: "test@example.com", name: "john" },
            content: "foo3",
          },
          {
            id: 4,
            articleId: 1,
            author: { id: 1, email: "test@example.com", name: "john" },
            content: "foo4",
          },
        ],
      },
    });
    loggedOutStore = setupStore({ auth: { claims: null, loginStatus: "LOGGED_OUT" } });
    jest.clearAllMocks();
  });

  describe("fetching comment", () => {
    it("should fetch comments list", async () => {
      axios.get = jest.fn().mockImplementation((url: string) => {
        if (url.startsWith(API_URL_COMMENTS)) {
          return Promise.resolve({ data: stubCommentDataList });
        } else {
          return Promise.resolve({ data: stubUserData });
        }
      });
      await store.dispatch(fetchCommentList(1));
      expect(store.getState().comment.commentList).toHaveLength(4);
    });
  });

  describe("creating comment", () => {
    it("should create a comment", async () => {
      axios.post = jest.fn().mockResolvedValue({ data: { ...stubCommentData, id: 999 } });
      await store.dispatch(createComment({ articleId: 1, content: stubCommentData.content }));
      expect(store.getState().comment.commentList).toHaveLength(5);
    });

    it("should not add to comment list if its article is different", async () => {
      axios.post = jest.fn().mockResolvedValue({ data: { ...stubCommentData, id: 999, article_id: 999 } });
      await store.dispatch(createComment({ articleId: 999, content: stubCommentData.content }));
      expect(store.getState().comment.commentList).toHaveLength(4);
    });

    it("should not create a comment when user is not logged in", async () => {
      axios.post = jest.fn();
      await loggedOutStore.dispatch(createComment({ articleId: 1, content: stubCommentData.content }));
      expect(axios.post).not.toBeCalled();
    });
  });

  describe("updating comment", () => {
    it("should update a comment", async () => {
      const newStubComment: CommentData = { ...stubCommentData, content: "barbar" };
      axios.patch = jest.fn().mockResolvedValue({ data: newStubComment });
      await store.dispatch(updateComment({ id: newStubComment.id, content: newStubComment.content }));
      expect(store.getState().comment.commentList.filter((c) => c.id === 1)[0].content).toEqual("barbar");
    });

    it("should not update a comment when user is not logged in", async () => {
      axios.patch = jest.fn();
      await loggedOutStore.dispatch(
        updateComment({ id: stubCommentData.id, content: stubCommentData.content })
      );
      expect(axios.patch).not.toBeCalled();
    });
  });

  describe("deleteing comment", () => {
    it("should delete a comment", async () => {
      axios.delete = jest.fn();
      await store.dispatch(deleteComment(1));
      expect(axios.delete).toBeCalled();
      expect(store.getState().comment.commentList.filter((c) => c.id === 1)).toHaveLength(0);
    });

    it("should not delete a comment when user is not logged in", async () => {
      axios.delete = jest.fn();
      await loggedOutStore.dispatch(deleteComment(1));
      expect(axios.delete).not.toBeCalled();
    });

    it("should delete comments when the related article is deleted", async () => {
      axios.delete = jest.fn();
      axios.get = jest.fn().mockResolvedValue({ data: [] });
      await store.dispatch(deleteArticle(1));
      expect(store.getState().comment.commentList.filter((c) => c.articleId === 1)).toHaveLength(0);
    });
  });
});
