import { fireEvent, screen, waitFor, within } from "@testing-library/react";
import ArticleEdit from "../../pages/ArticleEdit";
import { ArticleState, getArtilceInitialState } from "../../store/slices/article";
import { getAuthInitialState } from "../../store/slices/auth";
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

const mockUnwrapResult = jest.fn();
jest.mock("@reduxjs/toolkit", () => ({
  ...jest.requireActual("@reduxjs/toolkit"),
  unwrapResult: () => mockUnwrapResult(),
}));

const stubArticleState: ArticleState = {
  ...getArtilceInitialState(),
  selectedArticle: {
    id: 1,
    title: "foo",
    author: { id: 1, email: "test@example.com", name: "john" },
    content: "bar",
  },
  writingArticle: {
    title: "foo",
    content: "bar",
  },
};

describe("article edit page", () => {
  beforeEach(() => {
    jest.clearAllMocks();
  });

  it("should render", () => {
    const { container } = renderWithProviders(<ArticleEdit />);
    expect(container).toBeTruthy();
  });

  it("should dispatch", () => {
    renderWithProviders(<ArticleEdit />, {
      path: "/:articleId",
      route: "/1",
      preloadedState: { article: stubArticleState },
    });
    expect(mockDispatch).toBeCalledTimes(4);
  });

  it("should be able to go back to article page", () => {
    renderWithProviders(<ArticleEdit />, { preloadedState: { article: stubArticleState } });
    fireEvent.click(screen.getByRole("button", { name: /Back/i }));
    expect(mockNavigate).toBeCalledWith("/articles/1");
  });

  it("should show confirm message on going back to article page when the article is modified", () => {
    renderWithProviders(<ArticleEdit />, {
      preloadedState: {
        article: { ...stubArticleState, writingArticle: { title: "fooo", content: "baar" } },
      },
    });
    jest.spyOn(window, "confirm").mockImplementationOnce((message?: string) => true);
    fireEvent.click(screen.getByRole("button", { name: /Back/i }));
    expect(window.confirm).toBeCalledWith("Are you sure? The change will be lost.");
    expect(mockNavigate).toBeCalledTimes(1);
    expect(mockNavigate).toBeCalledWith("/articles/1");

    jest.spyOn(window, "confirm").mockImplementationOnce((message?: string) => false);
    fireEvent.click(screen.getByRole("button", { name: /Back/i }));
    expect(window.confirm).toBeCalledWith("Are you sure? The change will be lost.");
    expect(mockNavigate).toBeCalledTimes(1);
  });

  describe("article view mode", () => {
    it("should be able to switch to 'preview' mode", () => {
      renderWithProviders(<ArticleEdit />, {
        preloadedState: { article: { ...getArtilceInitialState(), writingArticleView: "WRITE" } },
      });
      fireEvent.click(screen.getByRole("button", { name: /Preview/i }));
      expect(mockDispatch).toBeCalledTimes(2);
    });

    it("should be able to switch to 'write' mode", () => {
      renderWithProviders(<ArticleEdit />, {
        preloadedState: { article: { ...getArtilceInitialState(), writingArticleView: "PREVIEW" } },
      });
      fireEvent.click(screen.getByRole("button", { name: /Write/i }));
      expect(mockDispatch).toBeCalledTimes(2);
    });

    it("should render article preview on 'preview' mode", () => {
      renderWithProviders(<ArticleEdit />, {
        preloadedState: {
          auth: {
            ...getAuthInitialState(),
            claims: {
              id: 1,
              email: "test@example.com",
              name: "john",
            },
          },
          article: {
            ...stubArticleState,
            writingArticleView: "PREVIEW",
          },
        },
      });
      const { getByText } = within(screen.getByRole("article"));
      expect(getByText("john")).toBeInTheDocument();
      expect(getByText("foo")).toBeInTheDocument();
      expect(getByText("bar")).toBeInTheDocument();
    });
  });

  describe("article edit confirm button", () => {
    it("should handle article edit confirm button", async () => {
      renderWithProviders(<ArticleEdit />, { preloadedState: { article: stubArticleState } });
      mockUnwrapResult.mockReturnValueOnce({ id: 1, title: "foo", author: null, content: "bar" });
      fireEvent.click(screen.getByRole("button", { name: /Confirm/i }));
      fireEvent.click(screen.getByRole("button", { name: /Confirm/i }));
      await waitFor(() => {
        expect(mockNavigate).toBeCalledWith("/articles/1");
      });
      await waitFor(() => {
        expect(mockNavigate).toBeCalledTimes(1);
      });
    });

    it("should disable button when writing article is empty", () => {
      renderWithProviders(<ArticleEdit />);
      expect(screen.getByRole("button", { name: /Confirm/i })).toBeDisabled();
    });
  });
});
