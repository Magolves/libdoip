/**
 * @brief Example DoIPServerModel with custom callbacks
 *
 */

#ifndef EXAMPLEDOIPSERVERMODEL_H
#define EXAMPLEDOIPSERVERMODEL_H

#include "DoIPServerModel.h"
#include "DoIPDownstreamServerModel.h"
#include "ThreadSafeQueue.h"
#include "uds/UdsMockProvider.h"
#include "uds/UdsResponseCode.h"

using namespace doip;
using namespace doip::uds;

class ExampleDoIPServerModel : public DoIPDownstreamServerModel {
  public:
    ExampleDoIPServerModel() : DoIPDownstreamServerModel("exmod", m_uds) {
        // Customize callbacks if needed

    }

  private:
    uds::UdsMockProvider m_uds;
};

#endif /* EXAMPLEDOIPSERVERMODEL_H */
