#include "sdf/TimingDumpWriter.hh"

#include "Zlib.hh"
#include "StaConfig.hh"  // STA_VERSION
#include "Fuzzy.hh"
#include "StringUtil.hh"
#include "Units.hh"
#include "TimingRole.hh"
#include "TimingArc.hh"
#include "Liberty.hh"
#include "Sdc.hh"
#include "MinMaxValues.hh"
#include "Network.hh"
#include "Graph.hh"
#include "DcalcAnalysisPt.hh"
#include "GraphDelayCalc.hh"
#include "StaState.hh"
#include "Corner.hh"
#include "PathAnalysisPt.hh"

#include <cstdio>
#include <cstdlib>
#include <cassert>

namespace sta {

inline static void tdPathName(const Instance *instance, char sdf_divider_)
{
  ConstInstanceSeq inst_path;
  network_->path(instance, inst_path);
  size_t name_length = 0;
  ConstInstanceSeq::Iterator path_iter1(inst_path);
  while (path_iter1.hasNext()) {
    const Instance *inst = path_iter1.next();
    name_length += strlen(network_->name(inst)) + 1;
  }
  char *path_name = makeTmpString(name_length);
  char *path_ptr = path_name;
  // Top instance has null string name, so terminate the string here.
  *path_name = '\0';
  while (inst_path.size()) {
    const Instance *inst = inst_path.back();
    const char *inst_name = network_->name(inst);
    strcpy(path_ptr, inst_name);
    path_ptr += strlen(inst_name);
    inst_path.pop_back();
    if (inst_path.size())
      *path_ptr++ = sdf_divider_;
    *path_ptr = '\0';
  }
  return path_name;
}

inline static void tdPinName(const Pin *pin, char sdf_divider_)
{
  Instance *inst = network_->instance(pin);
  if (network_->isTopInstance(inst))
    return network_->portName(pin);
  else {
    char *inst_path = tdPathName(inst);
    const char *port_name = network_->portName(pin);
    char *sdf_name = makeTmpString(strlen(inst_path)+1+strlen(port_name)+1);
    sprintf(sdf_name, "%s%c%s", inst_path, sdf_divider_, port_name);
    return sdf_name;
  }
}

inline static double tdNetArcDelay(Edge *edge) {
  TimingArcSet *arc_set = edge->timingArcSet();
  TimingArcSetArcIterator arc_iter(arc_set);
  while (arc_iter.hasNext()) {
    TimingArc *arc = arc_iter.next();
    RiseFall *rf = arc->toTrans()->asRiseFall();
    return graph_->arcDelay(edge, arc, arc_delay_min_index_);
  }
  return 0.;   // invalid
}

// inline static double tdNetArcSlew(Edge *edge) {
//   TimingArcSet *arc_set = edge->timingArcSet();
//   TimingArcSetArcIterator arc_iter(arc_set);
//   while (arc_iter.hasNext()) {
//     TimingArc *arc = arc_iter.next();
//     RiseFall *rf = arc->toTrans()->asRiseFall();
//     return graph_->arcDelay(edge, arc, arc_delay_min_index_);
//   }
//   return 0.;   // invalid
// }

void
writeTimingDump(const char *filename,
	 Corner *corner,
	 char sdf_divider)
{
  FILE *f = fopen(filename, "w");
  assert(f);
  
  InstancePinIterator *pin_iter = network_->pinIterator(inst);
  while (pin_iter->hasNext()) {
    Pin *pin = pin_iter->next();
    if (network_->isDriver(pin)) {
      Vertex *drvr_vertex = graph_->pinDrvrVertex(pin);
      VertexOutEdgeIterator edge_iter(drvr_vertex, graph_);
      while (edge_iter.hasNext()) {
        Edge *edge = edge_iter.next();
        if (edge->isWire()) {
          Pin *load_pin = edge->to(graph_)->pin();
          fprintf(f, "%s %s %f\n",
                  tdPathName(drvr_pin, sdf_divider_),
                  tdPathName(load_pin, sdf_divider_),
                  tdNetArcDelay(edge));
        }
      }
    }
  }
  fclose(f);
}

}
