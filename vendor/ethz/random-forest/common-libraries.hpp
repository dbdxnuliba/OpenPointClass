#ifndef LIBLEARNING_RANDOMFOREST_COMMON_LIBRARIES_H
#define LIBLEARNING_RANDOMFOREST_COMMON_LIBRARIES_H
#include <algorithm>
#include <numeric>
#include <unordered_set>
#include <random>
#include <limits>
#include <list>
#include <iostream>
#include <cstdio>

#include "../dataview.h"

namespace liblearning {
namespace RandomForest {

typedef std::vector< std::pair<float, int> > FeatureClassDataFloat;
inline void init_feature_class_data(FeatureClassDataFloat& data, int /*n_classes*/, int n_samples)
{
    data.resize(n_samples);
}
typedef std::unordered_set<int> FeatureSet;

typedef std::uniform_int_distribution<> UniformIntDist;
typedef std::normal_distribution<> NormalDist;
typedef std::mt19937 RandomGen;
typedef std::uniform_real_distribution<float> UnitDist;

struct ForestParams { 
    size_t n_classes;
    size_t n_features;
    size_t n_samples;
    size_t n_in_bag_samples;
    size_t max_depth;
    size_t n_trees;
    size_t min_samples_per_node;
    float  sample_reduction;

    // not used by RF
    double resolution; 
    double radius;
    int numScales;

    ForestParams() :
        n_classes(0),
        n_features(0),
        n_samples(0),
        n_in_bag_samples(0),
        max_depth(42),
        n_trees(100),
        min_samples_per_node(5),
        sample_reduction(0),
        resolution(-1),
        radius(0.6),
        numScales(5)
    {}

    void write (std::ostream& os){
      os.write((char*)(&n_classes), sizeof(size_t));
      os.write((char*)(&n_features), sizeof(size_t));
      os.write((char*)(&n_samples), sizeof(size_t));
      os.write((char*)(&n_in_bag_samples), sizeof(size_t));
      os.write((char*)(&max_depth), sizeof(size_t));
      os.write((char*)(&n_trees), sizeof(size_t));
      os.write((char*)(&min_samples_per_node), sizeof(size_t));
      os.write((char*)(&sample_reduction), sizeof(float));
      os.write((char*)(&resolution), sizeof(double));
      os.write((char*)(&radius), sizeof(double));
      os.write((char*)(&numScales), sizeof(int));
    }

    void read (std::istream& is){
      is.read((char*)(&n_classes), sizeof(size_t));
      is.read((char*)(&n_features), sizeof(size_t));
      is.read((char*)(&n_samples), sizeof(size_t));
      is.read((char*)(&n_in_bag_samples), sizeof(size_t));
      is.read((char*)(&max_depth), sizeof(size_t));
      is.read((char*)(&n_trees), sizeof(size_t));
      is.read((char*)(&min_samples_per_node), sizeof(size_t));
      is.read((char*)(&sample_reduction), sizeof(float));
      is.read((char*)(&resolution), sizeof(double));
      is.read((char*)(&radius), sizeof(double));
      is.read((char*)(&numScales), sizeof(int));
    }
};

struct QuadraticSplitter {
    typedef float FeatureType;
    typedef FeatureClassDataFloat FeatureClassData;
    int n_features;
    std::vector<FeatureType> w;
    FeatureType threshold;
    QuadraticSplitter() : n_features(0) {}
    QuadraticSplitter(int n_features, std::vector<FeatureType> const& w) :
        n_features(n_features), w(w)
    {}
    void set_threshold(FeatureType new_threshold) {
        threshold = new_threshold;
    }
    FeatureType map_sample(FeatureType const* v) const {
        double result = 0.0;
        int i_feature = 0;
        for (; i_feature < n_features; ++i_feature) {
            result += w[i_feature] * v[i_feature];
        }
        for (int i_1 = 0; i_1 < n_features; ++i_1) {
            for (int i_2 = 0; i_2 < n_features; ++i_2) {
                result += w[i_feature++] * v[i_1] * v[i_2];
            }
        }
        return result;
    }
    bool classify_sample(FeatureType const* v) const {
        return map_sample(v) > threshold;
    }
    void map_points(DataView2D<FeatureType> samples,
                    DataView2D<int>   labels,
                    int const*      sample_idxes,
                    int             n_samples,
                    FeatureClassData&        data_points) const
    {
        for (int i_sample = 0; i_sample < n_samples; ++i_sample) {
            int sample_idx    = sample_idxes[i_sample];
            int sample_class  = labels(sample_idx, 0);
            FeatureType sample_fval = map_sample(samples.row_pointer(sample_idx));
            data_points[i_sample] = std::make_pair(sample_fval, sample_class);
        }
    }
};

struct LinearSplitter {
    typedef float FeatureType;
    typedef FeatureClassDataFloat FeatureClassData;
    std::vector<FeatureType> w;
    FeatureType threshold;
    LinearSplitter() {}
    LinearSplitter(std::vector<FeatureType> const& w) :
        w(w)
    {}
    void set_threshold(FeatureType new_threshold) {
        threshold = new_threshold;
    }
    bool classify_sample(FeatureType const* v) const {
        return std::inner_product(w.begin(), w.end(), v, 0.0f) > threshold;
    }
    void map_points(DataView2D<FeatureType> samples,
                    DataView2D<int>   labels,
                    int const*      sample_idxes,
                    int             n_samples,
                    FeatureClassData&        data_points) const
    {
        for (int i_sample = 0; i_sample < n_samples; ++i_sample) {
            int sample_idx    = sample_idxes[i_sample];
            int sample_class  = labels(sample_idx, 0);
            FeatureType sample_fval = std::inner_product(w.begin(), w.end(), 
                                                   samples.row_pointer(sample_idx), 0.0f);
            data_points[i_sample] = std::make_pair(sample_fval, sample_class);
        }
    }
};

struct AxisAlignedSplitter {
    typedef float FeatureType;
    typedef FeatureClassDataFloat FeatureClassData;
    int feature;
    FeatureType threshold;
    AxisAlignedSplitter() : feature(-1), threshold(-1) {}
    AxisAlignedSplitter(int feature) : 
        feature(feature), threshold(-1)
    {}
    void set_threshold(FeatureType new_threshold) {
        threshold = new_threshold;
    }
    bool classify_sample(FeatureType const* v) const {
        return v[feature] > threshold;
    }
    void map_points(DataView2D<FeatureType> samples,
                    DataView2D<int>   labels,
                    int const*      sample_idxes,
                    int             n_samples,
                    FeatureClassData&        data_points) const
    {
        for (int i_sample = 0; i_sample < n_samples; ++i_sample) {
            // determine index of this sample ...
            int sample_idx    = sample_idxes[i_sample];
            // determine class ...
            int sample_class  = labels(sample_idx, 0);
            // determine value of the selected feature for this sample
            FeatureType sample_fval = samples(sample_idx, feature);
            data_points[i_sample] = std::make_pair(sample_fval, sample_class);
        }
    }

    void write (std::ostream& os){
      os.write((char*)(&feature), sizeof(int));
      os.write((char*)(&threshold), sizeof(FeatureType));
    }

    void read (std::istream& is){
      is.read((char*)(&feature), sizeof(int));
      is.read((char*)(&threshold), sizeof(FeatureType));
    }
};

struct AxisAlignedRandomSplitGenerator {
    typedef float FeatureType;
    FeatureSet features;
    FeatureSet::const_iterator it;

    void init(DataView2D<FeatureType> samples,
              DataView2D<int>   /*labels*/,
              int*            /*sample_idxes*/,
              int             /*n_samples*/,
              size_t          /*n_classes*/,
              RandomGen&      gen)
    {
        features.clear();
        int n_features = samples.cols;
        size_t n_used_features = std::sqrt(n_features);
        UniformIntDist dist(0, n_features - 1);
        // insert into set until required number of unique features is found
        while (features.size() < n_used_features) {
            features.insert(dist(gen));
        }
        it = features.begin();
    }
    AxisAlignedSplitter gen_proposal(RandomGen& /*gen*/)
    {
        if (it == features.end()) {
            it = features.begin();
        }
        return AxisAlignedSplitter(*it++);
    }
    size_t num_proposals() const {
        return features.size();
    }
};

struct LinearSplitGenerator {
    typedef float FeatureType;
    size_t n_features;
    size_t n_proposals;
    LinearSplitGenerator(size_t n_proposals = 5) : 
        n_proposals(n_proposals)
    {}
    void init(DataView2D<FeatureType> samples,
              DataView2D<int>   /*labels*/,
              int*            /*sample_idxes*/,
              int             /*n_samples*/,
              size_t          /*n_classes*/,
              RandomGen&      /*gen*/)
    {
        n_features = samples.cols;
    }
    size_t num_proposals() const {
        return n_proposals;
    }
    LinearSplitter gen_proposal(RandomGen& gen) {
        NormalDist dist;
        std::vector<FeatureType> weights(n_features);
        for (size_t i_feature = 0; i_feature < n_features; ++i_feature) {
            weights[i_feature] = dist(gen);
        }
        return LinearSplitter(weights);
    }
};

struct QuadraticSplitGenerator {
    typedef float FeatureType;
    size_t n_features;
    size_t n_proposals;
    QuadraticSplitGenerator(size_t n_proposals = 5) : 
        n_proposals(n_proposals)
    {}
    void init(DataView2D<FeatureType> samples,
              DataView2D<int>   /*labels*/,
              int*            /*sample_idxes*/,
              int             /*n_samples*/,
              size_t          /*n_classes*/,
              RandomGen&      /*gen*/)
    {
        n_features = samples.cols;
    }
    size_t num_proposals() const {
        return n_proposals;
    }
    QuadraticSplitter gen_proposal(RandomGen& gen) {
        NormalDist dist;
        std::vector<FeatureType> weights(n_features + n_features*n_features);
        for (size_t i_feature = 0; i_feature < weights.size(); ++i_feature) {
            weights[i_feature] = dist(gen);
        }
        return QuadraticSplitter(n_features, weights);
    }
};

}
}

#endif
