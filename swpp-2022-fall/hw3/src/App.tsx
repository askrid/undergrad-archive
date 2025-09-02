import { createBrowserRouter, Outlet, RouterProvider } from "react-router-dom";
import AuthProvider from "./components/AuthProvider";
import Container from "./components/Container";
import Header from "./components/Header";
import Article from "./pages/Article";
import ArticleCreate from "./pages/ArticleCreate";
import ArticleEdit from "./pages/ArticleEdit";
import Articles from "./pages/Articles";
import Login from "./pages/Login";

const router = createBrowserRouter([
  {
    path: "/",
    errorElement: <AuthProvider page="ROOT" />,
    children: [
      // Root page
      {
        path: "",
        element: (
          <AuthProvider page="ROOT">
            <Outlet />
          </AuthProvider>
        ),
      },

      // Login page
      {
        element: (
          <AuthProvider page="LOGIN">
            <Container>
              <Outlet />
            </Container>
          </AuthProvider>
        ),
        children: [
          {
            path: "login",
            element: <Login />,
          },
        ],
      },

      // Default pages
      {
        element: (
          <AuthProvider page="DEFAULT">
            <Header />
            <Container>
              <Outlet />
            </Container>
          </AuthProvider>
        ),
        children: [
          {
            path: "articles",
            element: <Articles />,
          },
          {
            path: "articles/create",
            element: <ArticleCreate />,
          },
          {
            path: "articles/:articleId",
            element: <Article />,
          },
          {
            path: "articles/:articleId/edit",
            element: <ArticleEdit />,
          },
        ],
      },
    ],
  },
]);

const App = () => {
  return <RouterProvider router={router} />;
};

export default App;
