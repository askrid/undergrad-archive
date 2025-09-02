import { fireEvent, screen, within } from "@testing-library/react";
import Article from "../../pages/Article";
import { ArticleState, getArtilceInitialState } from "../../store/slices/article";
import { AuthState, getAuthInitialState } from "../../store/slices/auth";
import { CommentState, getCommentInitialState } from "../../store/slices/comment";
import { renderWithProviders } from "../../test-utils";

const mockDispatch = jest.fn();
jest.mock("react-redux", () => ({
  ...jest.requireActual("react-redux"),
  useDispatch: () => mockDispatch,
}));

const mockNavigate = jest.fn();
jest.mock("react-router", () => ({
  ...jest.requireActual("react-router"),
  useNavigate: () => mockNavigate,
}));

const stubAuthState: AuthState = {
  ...getAuthInitialState(),
  claims: { id: 1, email: "test1@example.com", name: "alice" },
};
const stubArticleState: ArticleState = {
  ...getArtilceInitialState(),
  selectedArticle: { id: 1, title: "foo", author: stubAuthState.claims!, content: "bar" },
};
const stubCommentState: CommentState = {
  ...getCommentInitialState(),
  commentList: [
    { id: 1, articleId: 1, author: { id: 1, email: "test1@example.com", name: "alice" }, content: "foo1" },
    { id: 2, articleId: 1, author: { id: 2, email: "test2@example.com", name: "bob" }, content: "foo2" },
  ],
};

describe("article page", () => {
  beforeEach(() => {
    jest.clearAllMocks();
  });

  it("should render", () => {
    const { container } = renderWithProviders(<Article />);
    expect(container).toBeTruthy();
  });

  it("should dispatch", () => {
    renderWithProviders(<Article />, { path: "/:articleId", route: "/1" });
    expect(mockDispatch).toBeCalledTimes(2);
  });

  describe("article detail view", () => {
    it("should render selected article with its title, content, and author's name", () => {
      renderWithProviders(<Article />, { preloadedState: { article: stubArticleState } });
      expect(screen.getByText(stubArticleState.selectedArticle?.title!)).toBeInTheDocument();
      expect(screen.getByText(stubArticleState.selectedArticle?.content!)).toBeInTheDocument();
      expect(screen.getByText(stubArticleState.selectedArticle?.author.name!)).toBeInTheDocument();
    });

    it("should be able go back to articles page by hitting a button", () => {
      renderWithProviders(<Article />);
      fireEvent.click(screen.getByRole("button", { name: /Back/i }));
      expect(mockNavigate).toBeCalledWith("/articles");
    });

    describe("if the user is the author of the article", () => {
      it("should be able to go to edit page by hitting a button", () => {
        renderWithProviders(<Article />, {
          preloadedState: { auth: stubAuthState, article: stubArticleState },
        });
        fireEvent.click(screen.getByRole("button", { name: /Edit/i }));
        expect(mockNavigate).toBeCalledWith("/articles/1/edit");
      });

      it("should be able to delete the article by hitting a button", () => {
        renderWithProviders(<Article />, {
          preloadedState: { auth: stubAuthState, article: stubArticleState },
        });
        fireEvent.click(screen.getByRole("button", { name: /Delete/i }));
        expect(mockDispatch).toBeCalled();
        expect(mockNavigate).toBeCalledWith("/articles");
      });
    });
  });

  describe("comments list", () => {
    it("should render comments", () => {
      renderWithProviders(<Article />, { preloadedState: { comment: stubCommentState } });
      const commentList = screen.getByRole("list");
      const { getAllByRole, getByText } = within(commentList);
      expect(getAllByRole("listitem")).toHaveLength(stubCommentState.commentList.length);
      expect(getByText(stubCommentState.commentList[0].content)).toBeInTheDocument();
      expect(getByText(stubCommentState.commentList[0].author.name)).toBeInTheDocument();
    });

    describe("if the user is the author of one of the comments", () => {
      it("should be able to edit the comment", () => {
        jest.spyOn(window, "prompt").mockImplementation((message?: string) => "foobar");
        renderWithProviders(<Article />, {
          preloadedState: { auth: stubAuthState, comment: stubCommentState },
        });
        fireEvent.click(screen.getAllByRole("button", { name: /Edit/ })[0]);
        const comment = stubCommentState.commentList.filter(
          (c) => c.author.id === stubAuthState.claims!.id
        )[0];
        expect(window.prompt).toBeCalledWith(expect.anything(), comment.content);
        expect(mockDispatch).toBeCalled();
      });

      it("should not be able to edit the comment if the prompt is empty", () => {
        jest.spyOn(window, "prompt").mockImplementation((message?: string) => "");
        renderWithProviders(<Article />, {
          preloadedState: { auth: stubAuthState, comment: stubCommentState },
        });
        fireEvent.click(screen.getAllByRole("button", { name: /Edit/ })[0]);
        const comment = stubCommentState.commentList.filter(
          (c) => c.author.id === stubAuthState.claims!.id
        )[0];
        expect(window.prompt).toBeCalledWith(expect.anything(), comment.content);
        expect(mockDispatch).not.toBeCalled();
      });

      it("should be able to delete the comment", () => {
        renderWithProviders(<Article />, {
          preloadedState: { auth: stubAuthState, comment: stubCommentState },
        });
        fireEvent.click(screen.getAllByRole("button", { name: /Delete/i })[0]);
        expect(mockDispatch).toBeCalled();
      });
    });
  });

  describe("create comment form", () => {
    it("should have content input and submit button", () => {
      renderWithProviders(<Article />);
      expect(screen.getByRole("textbox")).toBeInTheDocument();
      expect(screen.getByRole("button", { name: /Submit/i })).toBeInTheDocument();
    });

    it("should handle comment submit", () => {
      renderWithProviders(<Article />, { preloadedState: { article: stubArticleState } });
      const submitButton = screen.getByRole("button", { name: /Submit/i });
      expect(submitButton).toBeDisabled();
      const contentInput = screen.getByRole("textbox");
      fireEvent.change(contentInput, { target: { value: "foobar" } });
      fireEvent.click(submitButton);
      expect(mockDispatch).toBeCalled();
    });
  });
});
