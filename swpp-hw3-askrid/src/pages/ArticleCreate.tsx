import { useEffect } from "react";
import { useDispatch, useSelector } from "react-redux";
import ArticleDetailPreview from "../components/ArticleDetailPreview";
import ArticleForm from "../components/ArticleForm";
import ArticleCreateConfirmButton from "../components/buttons/ArticleCreateConfirmButton";
import ArticleViewSelectButtons from "../components/buttons/ArticleViewSelectButtons";
import BackToArticlesButton from "../components/buttons/BackToArticlesButton";
import { AppDispatch } from "../store";
import { articleActions, selectArticle } from "../store/slices/article";

const ArticleCreate = () => {
  const { writingArticleView } = useSelector(selectArticle);
  const dispatch = useDispatch<AppDispatch>();

  useEffect(() => {
    dispatch(articleActions.clearWritingArticle());
    dispatch(articleActions.setWritingArticleView("WRITE"));
  }, [dispatch]);

  return (
    <main>
      <div className="font-semibold flex justify-between items-center mt-5 mb-4">
        <ArticleViewSelectButtons />
        <BackToArticlesButton elementId="back-create-article-button" />
      </div>
      <div className="mb-4">
        {writingArticleView === "WRITE" ? <ArticleForm /> : <ArticleDetailPreview />}
      </div>
      <div className="mb-16">
        <ArticleCreateConfirmButton />
      </div>
    </main>
  );
};

export default ArticleCreate;
