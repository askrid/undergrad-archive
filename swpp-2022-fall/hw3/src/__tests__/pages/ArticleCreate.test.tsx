import { fireEvent, screen, waitFor, within } from "@testing-library/react";
import ArticleCreate from "../../pages/ArticleCreate";
import { getArtilceInitialState } from "../../store/slices/article";
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

describe("article create page", () => {
  beforeEach(() => {
    jest.clearAllMocks();
  });

  it("should render", () => {
    const { container } = renderWithProviders(<ArticleCreate />);
    expect(container).toBeTruthy();
  });

  it("should dispatch", () => {
    renderWithProviders(<ArticleCreate />);
    expect(mockDispatch).toBeCalledTimes(2);
  });

  it("should be able to go back to articles page by hitting a button", () => {
    renderWithProviders(<ArticleCreate />);
    fireEvent.click(screen.getByRole("button", { name: /Back/i }));
    expect(mockNavigate).toBeCalledWith("/articles");
  });

  describe("article view mode", () => {
    it("should be able to switch to 'preview' mode", () => {
      renderWithProviders(<ArticleCreate />, {
        preloadedState: { article: { ...getArtilceInitialState(), writingArticleView: "WRITE" } },
      });
      fireEvent.click(screen.getByRole("button", { name: /Preview/i }));
      expect(mockDispatch).toBeCalledTimes(3);
    });

    it("should be able to switch to 'write' mode", () => {
      renderWithProviders(<ArticleCreate />, {
        preloadedState: { article: { ...getArtilceInitialState(), writingArticleView: "PREVIEW" } },
      });
      fireEvent.click(screen.getByRole("button", { name: /Write/i }));
      expect(mockDispatch).toBeCalledTimes(3);
    });

    it("should render article form on 'write' mode", () => {
      renderWithProviders(<ArticleCreate />, {
        preloadedState: { article: { ...getArtilceInitialState(), writingArticleView: "WRITE" } },
      });
      const [titleInput, contentInput] = screen.getAllByRole("textbox");
      fireEvent.change(titleInput, { target: { value: "foo" } });
      expect(mockDispatch).toBeCalledTimes(3);
      fireEvent.change(contentInput, { target: { value: "bar" } });
      expect(mockDispatch).toBeCalledTimes(4);
    });

    it("should render article preview on 'preview' mode", () => {
      renderWithProviders(<ArticleCreate />, {
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
            ...getArtilceInitialState(),
            writingArticle: { title: "foo", content: "bar" },
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

  describe("article create confirm button", () => {
    it("should handle article create confirm button", async () => {
      renderWithProviders(<ArticleCreate />, {
        preloadedState: {
          article: { ...getArtilceInitialState(), writingArticle: { title: "foo", content: "bar" } },
        },
      });
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
      renderWithProviders(<ArticleCreate />);
      expect(screen.getByRole("button", { name: /Confirm/i })).toBeDisabled();
    });
  });
});
