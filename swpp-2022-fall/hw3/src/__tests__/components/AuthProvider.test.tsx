import AuthProvider from "../../components/AuthProvider";
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

describe("auth provider", () => {
  beforeEach(() => {
    jest.clearAllMocks();
  });

  it("should render", () => {
    const { container } = renderWithProviders(<AuthProvider page="ROOT" />);
    expect(container).toBeTruthy();
  });

  it("should dispatch", () => {
    renderWithProviders(<AuthProvider page="ROOT" />);
    expect(mockDispatch).toBeCalled();
  });

  describe("navigation", () => {
    it("should navigate to articles page if user logged in on root page", () => {
      renderWithProviders(<AuthProvider page="ROOT" />, {
        preloadedState: { auth: { ...getAuthInitialState(), loginStatus: "LOGGED_IN" } },
      });
      expect(mockNavigate).toBeCalledWith("/articles", expect.anything());
    });

    it("should navigate to articles page if user logged in on login page", () => {
      renderWithProviders(<AuthProvider page="LOGIN" />, {
        preloadedState: { auth: { ...getAuthInitialState(), loginStatus: "LOGGED_IN" } },
      });
      expect(mockNavigate).toBeCalledWith("/articles", expect.anything());
    });

    it("should navigate to login page if user not logged in on root page", () => {
      renderWithProviders(<AuthProvider page="ROOT" />, {
        preloadedState: { auth: { ...getAuthInitialState(), loginStatus: "LOGGED_OUT" } },
      });
      expect(mockNavigate).toBeCalledWith("/login", expect.anything());
    });

    it("should navigate to login page if user not logged in on default page", () => {
      renderWithProviders(<AuthProvider page="DEFAULT" />, {
        preloadedState: { auth: { ...getAuthInitialState(), loginStatus: "LOGGED_OUT" } },
      });
      expect(mockNavigate).toBeCalledWith("/login", expect.anything());
    });

    it("shouldn't navigate if user logged in on default page", () => {
      renderWithProviders(<AuthProvider page="DEFAULT" />, {
        preloadedState: { auth: { ...getAuthInitialState(), loginStatus: "LOGGED_IN" } },
      });
      expect(mockNavigate).not.toBeCalled();
    });

    it("shouldn't navigate if user not logged in on login page", () => {
      renderWithProviders(<AuthProvider page="LOGIN" />, {
        preloadedState: { auth: { ...getAuthInitialState(), loginStatus: "LOGGED_OUT" } },
      });
      expect(mockNavigate).not.toBeCalled();
    });

    it("shouldn't navigate if user login status is loading", () => {
      renderWithProviders(<AuthProvider page="ROOT" />, {
        preloadedState: { auth: { ...getAuthInitialState(), loginStatus: "LOADING" } },
      });
      expect(mockNavigate).not.toBeCalled();
    });
  });
});
