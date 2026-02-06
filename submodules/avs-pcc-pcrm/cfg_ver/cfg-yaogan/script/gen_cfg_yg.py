
import copy
import yaml
from _io import open
import os
import errno
from collections import OrderedDict


def mkdir_p(path):
    try:
        os.makedirs(path)
    except OSError as exc:  # Python >2.5
        if exc.errno == errno.EEXIST and os.path.isdir(path):
            pass
        else:
            raise


def ordered_yaml_load(yaml_path, Loader=yaml.Loader,
                    object_pairs_hook=OrderedDict):
    class OrderedLoader(Loader):
        pass

    def construct_mapping(loader, node):
        loader.flatten_mapping(node)
        return object_pairs_hook(loader.construct_pairs(node))
    OrderedLoader.add_constructor(
        yaml.resolver.BaseResolver.DEFAULT_MAPPING_TAG,
        construct_mapping)
    with open(yaml_path) as stream:
        return yaml.load(stream, OrderedLoader)


def ordered_yaml_dump(data, stream=None, Dumper=yaml.SafeDumper, **kwds):
    class OrderedDumper(Dumper):
        pass

    def _dict_representer(dumper, data):
        return dumper.represent_mapping(
            yaml.resolver.BaseResolver.DEFAULT_MAPPING_TAG,
            data.items())
    OrderedDumper.add_representer(OrderedDict, _dict_representer)
    return yaml.dump(data, stream, OrderedDumper, **kwds)


def gen_cfg(condition,sequences_filename,folder_name):
    token = condition.split('-')

    f_condition = open('condition.yaml')
    data = ordered_yaml_load('condition.yaml')
    data_enc = data[condition]['encoder']
    data_dec = data[condition]['decoder']
    data_pcerror = data[condition]['pcerror']
    f_condition.close()

    f_ratepoint = open('ratepoint.yaml')
    rps = yaml.load(f_ratepoint, Loader=yaml.Loader)
    f_ratepoint.close()

    f_sequence = open(sequences_filename)
    seqs = yaml.load(f_sequence, Loader=yaml.Loader)
    f_sequence.close()

    f_profilelevel = open('profilelevel.yaml')
    pls = yaml.load(f_profilelevel, Loader=yaml.Loader)
    f_profilelevel.close()
    for seq_name in seqs:
        seq = seqs[seq_name]
        rps_geom = rps[token[1]][seq['category']]
        rps_attr = rps[token[2]][seq['category']]
        profile_level = pls[seq['category']]

        for r in rps_attr:
            dir = '{}/{}/{}/{}'.format(folder_name,condition, seq_name, r)
            mkdir_p(dir)

            # encoder cfg
            data_r = copy.deepcopy(data_enc)
            if len(rps_geom) == 1:
                rate_geom = rps_geom['']
            else:
                rate_geom = rps_geom[r]
            rate_attr = rps_attr[r]

            for key in rate_geom:
                if key in data_r and data_r[key] == '{}':
                    data_r[key] = rate_geom[key]
            for key in rate_attr:
                if key in data_r and data_r[key] == '{}':
                    data_r[key] = rate_attr[key]
            for key in seq:
                if key in data_r and data_r[key] == '{}':
                    data_r[key] = seq[key]
            for key in profile_level:
                if key in data_r and data_r[key] == '{}':
                    data_r[key] = profile_level[key]

            # remove empty arguments
            for key in list(data_r.keys()):
                if data_r[key] == '{}':
                    data_r.pop(key)

            with open('{}/encoder.cfg'.format(dir), 'w') as f:
                ordered_yaml_dump(data_r, f, default_flow_style=False)

            # decoder cfg
            data_r = copy.deepcopy(data_dec)
            with open('{}/decoder.cfg'.format(dir), 'w') as f:
                ordered_yaml_dump(data_r, f, default_flow_style=False)

            # pcerror cfg
            data_r = copy.deepcopy(data_pcerror)
            if len(rps_geom) == 1:
                rate_geom = rps_geom['']
            else:
                rate_geom = rps_geom[r]
            rate_attr = rps_attr[r]

            for key in rate_geom:
                if key in data_r and data_r[key] == '{}':
                    data_r[key] = rate_geom[key]
            for key in rate_attr:
                if key in data_r and data_r[key] == '{}':
                    data_r[key] = rate_attr[key]
            for key in seq:
                if key in data_r and data_r[key] == '{}':
                    data_r[key] = seq[key]

            # remove empty arguments
            for key in list(data_r.keys()):
                if data_r[key] == '{}':
                    data_r.pop(key)

            with open('{}/pcerror.cfg'.format(dir), 'w') as f:
                ordered_yaml_dump(data_r, f, default_flow_style=False)


if __name__ == "__main__":    
    folder_name="cfg_predict"
    sequences_filename = "sequences_predict_yg.yaml"
    gen_cfg("C4-losslessG-losslessA-ai",sequences_filename,folder_name)
    
    folder_name="cfg_predict"
    sequences_filename = "sequences_predtrans_limited_yg.yaml"
    gen_cfg("C1-limitlossyG-lossyA-ai",sequences_filename,folder_name)
    


