import { unwrapResult } from "@reduxjs/toolkit";
import { useDispatch, useSelector } from "react-redux";
import { useNavigate } from "react-router-dom";
import { AppDispatch } from "../../store";
import { createArticle, selectArticle } from "../../store/slices/article";

const ArticleCreateConfirmButton = () => {
  const dispatch = useDispatch<AppDispatch>();
  const navigate = useNavigate();
  const { writingArticle } = useSelector(selectArticle);

  const handleClick = async () => {
    // Unwrap the dispatch result to avoid race condition.
    const article = unwrapResult(await dispatch(createArticle(writingArticle)));

    if (article) {
      navigate(`/articles/${article.id}`);
    }
  };

  return (
    <button
      id="confirm-create-article-button"
      onClick={() => handleClick()}
      disabled={!writingArticle.content || !writingArticle.title}
      className="block py-2 px-4 w-full rounded bg-slate-800 text-white disabled:opacity-75 shadow-lg"
    >
      Confirm
    </button>
  );
};

export default ArticleCreateConfirmButton;
