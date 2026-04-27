import grpc
from concurrent import futures
import timesfm
import numpy as np
import timesfm_pb2
import timesfm_pb2_grpc

class TimesFMServicer(timesfm_pb2_grpc.TimesFMServicer):
    def __init__(self):
        print("Loading TimesFM...")
        self.model = timesfm.TimesFm.from_pretrained(
            checkpoint_repo_id="google/timesfm-2.5-200m"
        )

    def Predict(self, request, context):
        try:
            history = np.array(request.history, dtype=np.float32)
            horizon = request.horizon
            mean, lower, upper = self.model.predict(
                history[None, :], horizon=horizon
            )
            return timesfm_pb2.PredictResponse(
                mean=mean[0].tolist(),
                lower=lower[0].tolist(),
                upper=upper[0].tolist(),
                success=True,
                msg="ok"
            )
        except Exception as e:
            return timesfm_pb2.PredictResponse(
                success=False,
                msg=str(e)
            )

def serve():
    server = grpc.server(futures.ThreadPoolExecutor(10))
    timesfm_pb2_grpc.add_TimesFMServicer_to_server(TimesFMServicer(), server)
    server.add_insecure_port("[::]:50051")
    server.start()
    print("TimesFM gRPC server running on 50051")
    server.wait_for_termination()

if __name__ == "__main__":
    serve()