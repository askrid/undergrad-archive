import { fireEvent, screen, within } from "@testing-library/react";
import Articles from "../../pages/Articles";
import { getArtilceInitialState } from "../../store/slices/article";
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

describe("articles page", () => {
  beforeEach(() => {
    jest.clearAllMocks();
  });

  it("should render", () => {
    const { container } = renderWithProviders(<Articles />);
    expect(container).toBeTruthy();
  });

  it("should disptach", () => {
    renderWithProviders(<Articles />);
    expect(mockDispatch).toBeCalledTimes(1);
  });

  describe("go to article create button", () => {
    it("should render go to article create button", () => {
      renderWithProviders(<Articles />);
      expect(screen.getByRole("button", { name: /New Article/ })).toBeInTheDocument();
    });

    it("should navigate to article create page on click", () => {
      renderWithProviders(<Articles />);
      const button = screen.getByRole("button", { name: /New Article/ });
      fireEvent.click(button);
      expect(mockNavigate).toBeCalledWith("/articles/create");
    });
  });

  describe("article list", () => {
    it("should render articles in the list", () => {
      renderWithProviders(<Articles />, {
        preloadedState: {
          article: {
            ...getArtilceInitialState(),
            articleList: [
              {
                id: 1,
                title: "foo1",
                author: { id: 1, email: "test1@example.com", name: "alice" },
                content: "bar1",
              },
              {
                id: 2,
                title: "foo2",
                author: { id: 2, email: "test2@example.com", name: "bob" },
                content: "bar2",
              },
            ],
          },
        },
      });
      const articleList = screen.getByRole("list");
      const { getAllByRole, getByText } = within(articleList);
      expect(getAllByRole("listitem")).toHaveLength(2);
      expect(getByText("1")).toBeInTheDocument();
      expect(getByText("foo1")).toBeInTheDocument();
      expect(getByText("alice")).toBeInTheDocument();
      expect(getByText("2")).toBeInTheDocument();
      expect(getByText("foo2")).toBeInTheDocument();
      expect(getByText("bob")).toBeInTheDocument();
    });

    describe("article summary", () => {
      it("should navigate to detail page when clicked", () => {
        renderWithProviders(<Articles />, {
          preloadedState: {
            article: {
              ...getArtilceInitialState(),
              articleList: [
                {
                  id: 1,
                  title: "foo1",
                  author: { id: 1, email: "test1@example.com", name: "alice" },
                  content: "bar1",
                },
              ],
            },
          },
        });
        const articleSummary = screen.getAllByRole("article")[0];
        fireEvent.click(articleSummary);
        expect(mockNavigate).toBeCalledWith("/articles/1", expect.anything());

        const { getByRole } = within(articleSummary);
        fireEvent.click(getByRole("button"));
        expect(mockNavigate).toBeCalledWith("/articles/1", expect.anything());
      });
    });
  });
});
