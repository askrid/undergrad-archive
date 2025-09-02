import axios from "axios";
import { API_URL_ARTICLES } from "../../constants";
import { AppStore, setupStore } from "../../store";
import {
  articleActions,
  ArticleData,
  createArticle,
  deleteArticle,
  fetchArticleList,
  fetchSelectedArticle,
  updateArticle,
} from "../../store/slices/article";
import { UserData } from "../../store/slices/auth";

const stubArticleData: ArticleData = { id: 1, title: "foo1", author_id: 1, content: "bar1" };
const stubArticleDataList: ArticleData[] = [
  stubArticleData,
  { id: 2, title: "foo2", author_id: 1, content: "bar2" },
  { id: 3, title: "foo3", author_id: 1, content: "bar3" },
  { id: 4, title: "foo4", author_id: 1, content: "bar4" },
];
const stubUserData: UserData = {
  id: 1,
  email: "test@example.com",
  name: "john",
  password: "password",
  logged_in: true,
};

describe("article slice", () => {
  let store: AppStore;
  let loggedOutStore: AppStore;

  beforeEach(() => {
    store = setupStore({
      auth: { claims: { id: 1, email: "test@example.com", name: "john" }, loginStatus: "LOGGED_IN" },
      article: {
        articleList: [
          {
            id: 1,
            title: "foo1",
            author: { id: 1, email: "test@example.com", name: "john" },
            content: "bar1",
          },
          {
            id: 2,
            title: "foo2",
            author: { id: 2, email: "test2@example.com", name: "bob" },
            content: "bar2",
          },
          {
            id: 3,
            title: "foo3",
            author: { id: 1, email: "test@example.com", name: "john" },
            content: "bar3",
          },
          {
            id: 4,
            title: "foo4",
            author: { id: 2, email: "test2@example.com", name: "bob" },
            content: "bar4",
          },
        ],

        selectedArticle: {
          id: 1,
          title: "foo1",
          author: { id: 1, email: "test@example.com", name: "john" },
          content: "bar1",
        },
        writingArticle: {
          title: "foo",
          content: "bar",
        },
        writingArticleView: "WRITE",
      },
    });
    loggedOutStore = setupStore({ auth: { claims: null, loginStatus: "LOGGED_OUT" } });
    jest.clearAllMocks();
  });

  describe("fetching article", () => {
    it("should fetch articles list", async () => {
      axios.get = jest.fn().mockImplementation((url: string) => {
        if (url === API_URL_ARTICLES) {
          return Promise.resolve({ data: stubArticleDataList });
        } else {
          return Promise.resolve({ data: stubUserData });
        }
      });
      await store.dispatch(fetchArticleList());
      expect(store.getState().article.articleList).toHaveLength(4);
    });

    it("should fetch a single article", async () => {
      axios.get = jest
        .fn()
        .mockResolvedValueOnce({ data: stubArticleData })
        .mockResolvedValueOnce({ data: stubUserData });
      await store.dispatch(fetchSelectedArticle(1));
      expect(store.getState().article.selectedArticle?.id).toEqual(1);
    });

    it("should change selected article when fetching a different article", async () => {
      axios.get = jest
        .fn()
        .mockResolvedValueOnce({ data: stubArticleDataList.filter((a) => a.id === 2)[0] })
        .mockResolvedValueOnce({ data: stubUserData });
      await store.dispatch(fetchSelectedArticle(2));
      expect(store.getState().article.selectedArticle?.id).toEqual(2);
    });
  });

  describe("creating article", () => {
    it("should create an article", async () => {
      axios.post = jest.fn().mockResolvedValue({ data: { ...stubArticleData, id: 999 } });
      await store.dispatch(createArticle({ title: stubArticleData.title, content: stubArticleData.content }));
      expect(store.getState().article.articleList).toHaveLength(5);
    });

    it("should not create an article when user is not logged in", async () => {
      axios.post = jest.fn();
      await loggedOutStore.dispatch(
        createArticle({ title: stubArticleData.title, content: stubArticleData.content })
      );
      expect(axios.post).not.toBeCalled();
    });
  });

  describe("updating article", () => {
    it("should update an article", async () => {
      const newStubArticle: ArticleData = { ...stubArticleData, content: "barbar" };
      axios.patch = jest.fn().mockResolvedValue({ data: newStubArticle });
      await store.dispatch(
        updateArticle({ id: newStubArticle.id, title: newStubArticle.title, content: newStubArticle.content })
      );
      expect(store.getState().article.selectedArticle?.content).toEqual("barbar");
      expect(store.getState().article.articleList.filter((a) => a.id === 1)[0].content).toEqual("barbar");
    });

    it("should not update selected artilce if it is different article", async () => {
      const newStubArticle: ArticleData = { id: 999, title: "foo999", author_id: 1, content: "bar999" };
      axios.patch = jest.fn().mockResolvedValue({ data: newStubArticle });
      await store.dispatch(
        updateArticle({ id: newStubArticle.id, title: newStubArticle.title, content: newStubArticle.content })
      );
      expect(store.getState().article.selectedArticle?.id).toEqual(1);
      expect(store.getState().article.selectedArticle?.content).toEqual("bar1");
    });

    it("should not update an article when user is not logged in", async () => {
      axios.patch = jest.fn();
      await loggedOutStore.dispatch(
        updateArticle({
          id: stubArticleData.id,
          title: stubArticleData.title,
          content: stubArticleData.content,
        })
      );
      expect(axios.patch).not.toBeCalled();
    });
  });

  describe("deleting article", () => {
    it("should delete an article", async () => {
      axios.delete = jest.fn();
      axios.get = jest
        .fn()
        .mockResolvedValue({ data: [{ id: 1, article_id: 1, author_id: 1, content: "foo" }] });
      await store.dispatch(deleteArticle(1));
      expect(axios.delete).toBeCalled();
      expect(store.getState().article.selectedArticle).toBeNull();
      expect(store.getState().article.articleList.filter((a) => a.id === 1)).toHaveLength(0);
    });

    it("should not delete selected artilce if it is a different article", async () => {
      axios.delete = jest.fn();
      axios.get = jest.fn().mockResolvedValue({ data: [] });
      await store.dispatch(deleteArticle(999));
      expect(store.getState().article.selectedArticle).not.toBeNull();
      expect(store.getState().article.selectedArticle?.id).toEqual(1);
    });

    it("should not delete when user is not logged in", async () => {
      axios.delete = jest.fn();
      await loggedOutStore.dispatch(deleteArticle(1));
      expect(axios.delete).not.toBeCalled();
    });
  });

  describe("writing article", () => {
    it("should update writing article's title", () => {
      store.dispatch(articleActions.writeArticleTitle("foooooo"));
      expect(store.getState().article.writingArticle.title).toEqual("foooooo");
    });

    it("should update writing article's content", () => {
      store.dispatch(articleActions.writeArticleContent("baaaaaar"));
      expect(store.getState().article.writingArticle.content).toEqual("baaaaaar");
    });

    it("should clear writing article", () => {
      store.dispatch(articleActions.clearWritingArticle());
      expect(store.getState().article.writingArticle).toEqual({ title: "", content: "" });
    });

    it("should set writing article view mode", () => {
      store.dispatch(articleActions.setWritingArticleView("PREVIEW"));
      expect(store.getState().article.writingArticleView).toEqual("PREVIEW");
    });
  });
});
