import { useSelector } from "react-redux";
import { selectAuth } from "../store/slices/auth";
import { selectComment } from "../store/slices/comment";
import CommentDeleteButton from "./buttons/CommentDeleteButton";
import CommentEditButton from "./buttons/CommentEditButton";
import Comment from "./Comment";

const CommentList = () => {
  const { commentList } = useSelector(selectComment);
  const { claims } = useSelector(selectAuth);

  return (
    <div className="p-6 rounded bg-white shadow-lg">
      <h2 className="font-semibold italic text-2xl mb-4">Comments</h2>
      <ul className="space-y-5">
        {commentList.map((comment) => (
          <li key={`comment-${comment.id}`}>
            {claims?.id === comment.author.id && (
              <div className="text-end space-x-1 mb-1">
                <CommentEditButton commentId={comment.id} initialContent={comment.content} />
                <CommentDeleteButton commentId={comment.id} />
              </div>
            )}
            <Comment authorName={comment.author.name} content={comment.content} />
          </li>
        ))}
      </ul>
    </div>
  );
};

export default CommentList;
