
// /* 邻域信息 */
// typedef struct {
//   int *nbrCoord;  //邻域的坐标属性数组
//   int nbrSize;    //邻域大小
//   float *weights; //邻域的权重数组
// } NbrInfo;

kernel void add(global const float *a, global const float *b,
                global float *res) {
  int gid_x = get_global_id(0);
  int gid_y = get_global_id(1);
  int width = get_global_size(0);
  int height = get_global_size(1);
  int local_size_x = get_local_id(0);

  printf("local_size_x:%d\n", local_size_x);
  int id = gid_y * width + gid_x;
  // printf("gidx:%d , gidy:%d\n", gid_x, gid_y);
  res[id] = a[id] + b[id];
}

kernel void test(global unsigned char *inData, global unsigned char *outData,
                 global int *_width, global int *_height, global int *br,
                 global int *step, global int *_nbrCoord, global int *_nbrSize,
                 local int *nbrCoord) {
  int gid_x = get_global_id(0) * step[0];
  int gid_y = get_global_id(1) * step[1];
  int width = _width[0];
  int height = _height[0];
  int minX = br[0];
  int minY = br[1];
  int maxX = br[2];
  int maxY = br[3];
  int nbrSize = _nbrSize[0];
  for (int i = 0; i < 2 * nbrSize; i++)
    nbrCoord[i] = _nbrCoord[i];

  /* 循环邻域 */
  for (int y = gid_y; y < gid_y + step[1]; y++) {
    for (int x = gid_x; x < gid_x + step[0]; x++) {
      int index = y * width + x;
      if (x > maxX || x < minX || y > maxY || y < minY)
        continue;

      /* 对所有邻域栅格块循环 */
      unsigned char sum = 0;
      for (int i = 0; i < nbrSize; i++) {
        /* 根据相对位置找到邻域栅格的绝对位置 */
        int nbrIndex = index + nbrCoord[i * 2] + nbrCoord[i * 2 + 1] * width;
        if (inData[nbrIndex] > 0) {
          sum += inData[nbrIndex];
        }
      }
      outData[index] = sum;
    }
  }
}

kernel void rf_ca(global unsigned char *inData, global unsigned char *outData,
                  global int *_width, global int *_height, global int *br,
                  global int *step, global int *_nbrCoord, global int *_nbrSize,
                  local int *nbrCoord, global unsigned char *limitData,
                  global unsigned char *probData, global float *_thold) {
  int gid_x = get_global_id(0) * step[0];
  int gid_y = get_global_id(1) * step[1];
  int width = _width[0];
  int height = _height[0];
  int minX = br[0];
  int minY = br[1];
  int maxX = br[2];
  int maxY = br[3];
  int nbrSize = _nbrSize[0];
  float thold = _thold[0];
  for (int i = 0; i < 2 * nbrSize; i++)
    nbrCoord[i] = _nbrCoord[i];

  /* 循环邻域 */
  for (int y = gid_y; y < gid_y + step[1]; y++) {
    for (int x = gid_x; x < gid_x + step[0]; x++) {
      int index = y * width + x;
      if (x > maxX || x < minX || y > maxY || y < minY)
        continue;
      if (inData[index] > 1) //大于1 异常值
        continue;

      /* 对所有邻域栅格块循环 */
      unsigned char sum = 0;
      for (int i = 0; i < nbrSize; i++) {
        /* 根据相对位置找到邻域栅格的绝对位置 */
        int nbrIndex = index + nbrCoord[i * 2] + nbrCoord[i * 2 + 1] * width;
        if (nbrIndex != index) { //不加自己所在的邻域
          sum += inData[nbrIndex];
        }
      }
      float averageData = sum * 1.0 / (nbrSize - 1); //邻域均值

      /* 转换概率 */
      float trans_prob = averageData * (float)limitData[index] *
                         (float)probData[index] / 100.0;

      if (trans_prob > thold)
        outData[index] = 1;
      else
        outData[index] = inData[index];
    }
  }
}