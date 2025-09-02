import { useEffect } from "react";
import { useDispatch, useSelector } from "react-redux";
import { useNavigate } from "react-router-dom";
import { AppDispatch } from "../store";
import { getAuth, selectAuth } from "../store/slices/auth";

interface Props {
  page: "ROOT" | "LOGIN" | "DEFAULT";
  children?: React.ReactNode;
}

const AuthProvider = ({ page, children }: Props) => {
  const dispatch = useDispatch<AppDispatch>();
  const navigate = useNavigate();
  const auth = useSelector(selectAuth);

  // Get and store the current user's status.
  useEffect(() => {
    dispatch(getAuth());
  }, [dispatch]);

  useEffect(() => {
    // Redirect to "/articles" if the current user is logged in on root page or login page.
    if ((page === "ROOT" || page === "LOGIN") && auth.loginStatus === "LOGGED_IN") {
      navigate("/articles", { replace: true });
    }

    // Redirect to "/login" if the current user is not logged in on root page or default page.
    if ((page === "ROOT" || page === "DEFAULT") && auth.loginStatus === "LOGGED_OUT") {
      navigate("/login", { replace: true });
    }
  }, [page, navigate, auth]);

  return <>{children}</>;
};

export default AuthProvider;
